# ChampSim + Ramulator Integration Architecture

## Overall Strategy

Source files from both ChampSim and Ramulator are copied directly into the project:
- `source/ChampSim/` and `include/ChampSim/` — ChampSim files (modified)
- `source/Ramulator/` and `include/Ramulator/` — Ramulator files (unmodified)

The original ChampSim code is preserved inside `#else` branches of `#if (USER_CODES == ENABLE)` guards throughout every modified file. This means both the original and the modified code coexist in every file; `include/ProjectConfiguration.h` selects between them at compile time by defining `USER_CODES (ENABLE)`. The Ramulator files are used as-is without modification.

---

## Key Files

| File | Role |
|---|---|
| `include/ProjectConfiguration.h` | Master compile-time switch file |
| `include/ChampSim/dram_controller.h` | Core integration point: templated MEMORY_CONTROLLER |
| `include/ChampSim/core_inst.h` | Wires all hardware modules into generated_environment |
| `source/main.cc` | Entry point; resolves DRAM type at runtime via template dispatch |
| `include/main.h` | Includes all ChampSim and Ramulator headers together |

---

## Compile-Time Configuration (`ProjectConfiguration.h`)

Two macros gate the entire Ramulator integration:

- `RAMULATOR (ENABLE/DISABLE)` — whether to use Ramulator at all
- `MEMORY_USE_HYBRID (ENABLE/DISABLE)` — whether to use two memory tiers

These affect which template specialization of `MEMORY_CONTROLLER` and `generated_environment` is compiled in.

---

## Core Integration Point: `MEMORY_CONTROLLER` Template

### Original ChampSim
ChampSim's `MEMORY_CONTROLLER` is a plain class with its own internal DRAM timing model.

### This Project
`include/ChampSim/dram_controller.h` replaces it with a template class that holds references to live Ramulator instances:

```
// Single memory
template<typename T>
class MEMORY_CONTROLLER : public champsim::operable {
    ramulator::Memory<T, ramulator::Controller>& memory;
    ...
};

// Hybrid memory (two tiers)
template<typename T, typename T2>
class MEMORY_CONTROLLER : public champsim::operable {
    ramulator::Memory<T,  ramulator::Controller>& memory;   // fast
    ramulator::Memory<T2, ramulator::Controller>& memory2;  // slow
    ...
};
```

### Per-Cycle Operation
Each simulation cycle, ChampSim's event loop calls `MEMORY_CONTROLLER::operate()`, which:
1. Calls `memory.tick()` (and `memory2.tick()`) to advance Ramulator's internal DRAM timing by one cycle.
2. Drains the read/write queues received from the LLC.

### Request Flow (ChampSim → Ramulator)
When the LLC issues a memory access, `add_rq()` (read) or `add_wq()` (write) is called:
1. Converts the ChampSim `channel::request_type` packet into a `ramulator::Request`.
2. Binds `MEMORY_CONTROLLER::return_data()` as the completion callback on the request.
3. Calls `memory.send(request)` or `memory2.send(request)`.

### Completion Flow (Ramulator → ChampSim)
When Ramulator finishes processing a request, it fires the bound callback:
1. `return_data(ramulator::Request& request)` is called.
2. It looks up the original ChampSim packet and marks it complete.
3. The response travels back up the cache hierarchy to the CPU.

---

## Address Partitioning (Hybrid Memory)

The physical address space is partitioned linearly:

```
[0,  memory.max_address)                           → fast memory (T)
[memory.max_address,  memory.max_address + memory2.max_address)  → slow memory (T2)
```

`MEMORY_CONTROLLER` inspects the physical address of every incoming packet to decide which `memory.send()` to call. No address remapping is done at the Ramulator level; Ramulator always sees addresses starting from 0 within its own space (slow memory addresses have `memory.max_address` subtracted before being passed to `memory2.send()`).

---

## OS-Transparent Management Layer

When `MEMORY_USE_OS_TRANSPARENT_MANAGEMENT (ENABLE)`, `MEMORY_CONTROLLER` also holds an `OS_TRANSPARENT_MANAGEMENT` object (defined in `include/os_transparent_management.h`). It is constructed with the total address space size and the fast-memory boundary:

```cpp
os_transparent_management(
    *(new OS_TRANSPARENT_MANAGEMENT(
        memory.max_address + memory2.max_address,  // total
        memory.max_address                         // fast memory capacity
    ))
)
```

This layer intercepts every LLC miss inside `add_rq()`/`add_wq()` and decides:
- Which physical memory tier the address currently maps to.
- Whether to trigger a data migration (swap) between tiers.

Research implementations selectable via `ProjectConfiguration.h`:
- `IDEAL_LINE_LOCATION_TABLE` — CAMEO-style, cache-line granularity location table
- `COLOCATED_LINE_LOCATION_TABLE` — co-located metadata variant
- `IDEAL_VARIABLE_GRANULARITY` — variable-granularity migration regions
- `IDEAL_SINGLE_MEMPOD` — MemPod single-tier emulation

---

## Runtime DRAM Type Resolution (`source/main.cc`)

Ramulator's `Memory<T>` is a C++ template typed on the DRAM spec (e.g., `Memory<HBM>`). Because the spec is a compile-time type, the concrete type cannot be selected with a runtime `if`. Instead, `main.cc` resolves it through a cascade of template function instantiations:

```
main()
  reads standard string from .cfg file (e.g., "HBM")
  calls configure_fast_memory_to_run_simulation(standard, ...)
    if (standard == "HBM") → new ramulator::HBM(...)
      calls next_configure_slow_memory_to_run_simulation<HBM>(standard2, ..., hbm_spec)
        if (standard2 == "DDR4") → new ramulator::DDR4(...)
          calls start_run_simulation<HBM, DDR4>(configs, hbm_spec, configs2, ddr4_spec)
            builds ramulator::Memory<HBM, Controller> memory
            builds ramulator::Memory<DDR4, Controller> memory2
            calls run_simulation<HBM, DDR4>(configs, memory, configs2, memory2)
              constructs generated_environment<CHAMPSIM_BUILD, HBM, DDR4>(memory, memory2)
              calls champsim::main(gen_environment, phases, traces)
```

This is also why every DRAM standard must be listed explicitly in the `if/else if` chains — there is no runtime polymorphism over the DRAM type.

---

## Hardware Environment Assembly (`include/ChampSim/core_inst.h`)

`generated_environment<ID, MEMORY_TYPE, MEMORY_TYPE2>` is a template struct that owns and connects all simulated hardware. It is constructed by passing the live Ramulator `Memory` objects:

```
generated_environment<CHAMPSIM_BUILD, T, T2>(memory, memory2)
  ├── channels[]           — inter-component communication queues
  ├── MEMORY_CONTROLLER<T,T2>(memory, memory2)  — holds Ramulator references
  ├── VirtualMemory vmem
  ├── PageTableWalker ptws[]
  ├── CACHE caches[]       — DTLB, ITLB, L1D, L1I, L2C, STLB, LLC
  └── O3_CPU cores[]
```

When `RAMULATOR=DISABLE`, the template degenerates to `generated_environment<ID>` with no MEMORY_TYPE parameters, and the `MEMORY_CONTROLLER` reverts to ChampSim's original built-in DRAM model.

---

## Simulation Loop

After `generated_environment` is constructed, control passes to `champsim::main()` (ChampSim's unmodified simulation engine), which runs the event loop by repeatedly calling `operable_view()` to get all components and ticking them in order — CPUs, caches, MEMORY_CONTROLLER (which ticks Ramulator). The Warmup and Simulation phases are driven by `champsim::phase_info` objects populated from the command-line arguments.

---

## What Was NOT Changed in Ramulator

The entire `source/Ramulator/` and `include/Ramulator/` directory is used verbatim. The only integration surface Ramulator exposes is:
- `ramulator::Config` — parses the `.cfg` file
- `ramulator::Memory<T, Controller>::send(Request)` — enqueues a request
- `ramulator::Memory<T, Controller>::tick()` — advances one cycle
- `ramulator::Memory<T, Controller>::finish()` — flushes end-of-simulation stats
- `ramulator::Request` — carries the address, type (READ/WRITE), and a `std::function` callback

All complexity of the integration lives on the ChampSim side, inside `MEMORY_CONTROLLER`.
