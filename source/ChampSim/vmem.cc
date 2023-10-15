/*
 *    Copyright 2023 The ChampSim Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ChampSim/vmem.h"

#include "ProjectConfiguration.h" // User file

#if (USE_VCPKG == ENABLE)
#include <fmt/core.h>
#endif // USE_VCPKG

#include <cassert>

#include "ChampSim/champsim.h"
#include "ChampSim/champsim_constants.h"

#if (USER_CODES == ENABLE)

#if (RAMULATOR == ENABLE)
VirtualMemory::VirtualMemory(uint64_t page_table_page_size, std::size_t page_table_levels, uint64_t minor_penalty, std::size_t memory_size)
: next_ppage(VMEM_RESERVE_CAPACITY), last_ppage(1ull << (LOG2_PAGE_SIZE + champsim::lg2(page_table_page_size / PTE_BYTES) * page_table_levels)),
  minor_fault_penalty(minor_penalty), pt_levels(page_table_levels), pte_page_size(page_table_page_size)
{
    assert(page_table_page_size > 1024);
    assert(page_table_page_size == (1ull << champsim::lg2(page_table_page_size)));
    assert(last_ppage > VMEM_RESERVE_CAPACITY);

    auto required_bits = champsim::lg2(last_ppage);
#if (USE_VCPKG == ENABLE)
    if (required_bits > 64)
    {
        fmt::print("WARNING: virtual memory configuration would require {} bits of addressing.\n", required_bits);
    } // LCOV_EXCL_LINE
    if (required_bits > champsim::lg2(memory_size))
    {
        fmt::print("WARNING: physical memory size is smaller than virtual memory size.\n");
    } // LCOV_EXCL_LINE

#endif // USE_VCPKG

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    if (required_bits > 64)
    {
        std::fprintf(output_statistics.file_handler, "WARNING: virtual memory configuration would require %d bits of addressing.\n", required_bits);
    } // LCOV_EXCL_LINE
    if (required_bits > champsim::lg2(memory_size))
    {
        std::fprintf(output_statistics.file_handler, "WARNING: physical memory size is smaller than virtual memory size.\n");
    } // LCOV_EXCL_LINE

#endif // PRINT_STATISTICS_INTO_FILE
}

uint64_t VirtualMemory::shamt(std::size_t level) const { return LOG2_PAGE_SIZE + champsim::lg2(pte_page_size / PTE_BYTES) * (level - 1); }

uint64_t VirtualMemory::get_offset(uint64_t vaddr, std::size_t level) const
{
    return (vaddr >> shamt(level)) & champsim::bitmask(champsim::lg2(pte_page_size / PTE_BYTES));
}

uint64_t VirtualMemory::ppage_front() const
{
    assert(available_ppages() > 0);
    return next_ppage;
}

void VirtualMemory::ppage_pop() { next_ppage += PAGE_SIZE; }

std::size_t VirtualMemory::available_ppages() const { return (last_ppage - next_ppage) / PAGE_SIZE; }

std::pair<uint64_t, uint64_t> VirtualMemory::va_to_pa(uint32_t cpu_num, uint64_t vaddr)
{
    auto [ppage, fault] = vpage_to_ppage_map.insert({
        {cpu_num, vaddr >> LOG2_PAGE_SIZE},
        ppage_front()
    });

    // this vpage doesn't yet have a ppage mapping
    if (fault)
    {
        ppage_pop();

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
        output_statistics.virtual_page_count++;
#endif // PRINT_STATISTICS_INTO_FILE
    }

    auto paddr = champsim::splice_bits(ppage->second, vaddr, LOG2_PAGE_SIZE);
    if constexpr (champsim::debug_print)
    {
#if (USE_VCPKG == ENABLE)
        fmt::print("[VMEM] {} paddr: {:x} vaddr: {:x} fault: {}\n", __func__, paddr, vaddr, fault);
#endif // USE_VCPKG
    }

    return {paddr, fault ? minor_fault_penalty : 0};
}

std::pair<uint64_t, uint64_t> VirtualMemory::get_pte_pa(uint32_t cpu_num, uint64_t vaddr, std::size_t level)
{
    if (next_pte_page == 0)
    {
        next_pte_page = ppage_front();
        ppage_pop();
    }

    std::tuple key {cpu_num, vaddr >> shamt(level), level};
    auto [ppage, fault] = page_table.insert({key, next_pte_page});

    // this PTE doesn't yet have a mapping
    if (fault)
    {
        next_pte_page += pte_page_size;
        if (! (next_pte_page % PAGE_SIZE))
        {
            next_pte_page = ppage_front();
            ppage_pop();
        }

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
        output_statistics.valid_pte_count[level - 1]++;
#endif // PRINT_STATISTICS_INTO_FILE
    }

    auto offset = get_offset(vaddr, level);
    auto paddr  = champsim::splice_bits(ppage->second, offset * PTE_BYTES, champsim::lg2(pte_page_size));
    if constexpr (champsim::debug_print)
    {
#if (USE_VCPKG == ENABLE)
        fmt::print("[VMEM] {} paddr: {:x} vaddr: {:x} pt_page_offset: {} translation_level: {} fault: {}\n", __func__, paddr, vaddr, offset, level, fault);
#endif // USE_VCPKG
    }

    return {paddr, fault ? minor_fault_penalty : 0};
}

#else

VirtualMemory::VirtualMemory(uint64_t page_table_page_size, std::size_t page_table_levels, uint64_t minor_penalty, MEMORY_CONTROLLER& dram)
: next_ppage(VMEM_RESERVE_CAPACITY), last_ppage(1ull << (LOG2_PAGE_SIZE + champsim::lg2(page_table_page_size / PTE_BYTES) * page_table_levels)),
  minor_fault_penalty(minor_penalty), pt_levels(page_table_levels), pte_page_size(page_table_page_size)
{
    assert(page_table_page_size > 1024);
    assert(page_table_page_size == (1ull << champsim::lg2(page_table_page_size)));
    assert(last_ppage > VMEM_RESERVE_CAPACITY);

    auto required_bits = champsim::lg2(last_ppage);
#if (USE_VCPKG == ENABLE)
    if (required_bits > 64)
    {
        fmt::print("WARNING: virtual memory configuration would require {} bits of addressing.\n", required_bits);
    } // LCOV_EXCL_LINE
    if (required_bits > champsim::lg2(dram.size()))
    {
        fmt::print("WARNING: physical memory size is smaller than virtual memory size.\n");
    } // LCOV_EXCL_LINE

#endif // USE_VCPKG

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    if (required_bits > 64)
    {
        std::fprintf(output_statistics.file_handler, "WARNING: virtual memory configuration would require %d bits of addressing.\n", required_bits);
    } // LCOV_EXCL_LINE
    if (required_bits > champsim::lg2(dram.size()))
    {
        std::fprintf(output_statistics.file_handler, "WARNING: physical memory size is smaller than virtual memory size.\n");
    } // LCOV_EXCL_LINE

#endif // PRINT_STATISTICS_INTO_FILE
}

uint64_t VirtualMemory::shamt(std::size_t level) const { return LOG2_PAGE_SIZE + champsim::lg2(pte_page_size / PTE_BYTES) * (level - 1); }

uint64_t VirtualMemory::get_offset(uint64_t vaddr, std::size_t level) const
{
    return (vaddr >> shamt(level)) & champsim::bitmask(champsim::lg2(pte_page_size / PTE_BYTES));
}

uint64_t VirtualMemory::ppage_front() const
{
    assert(available_ppages() > 0);
    return next_ppage;
}

void VirtualMemory::ppage_pop() { next_ppage += PAGE_SIZE; }

std::size_t VirtualMemory::available_ppages() const { return (last_ppage - next_ppage) / PAGE_SIZE; }

std::pair<uint64_t, uint64_t> VirtualMemory::va_to_pa(uint32_t cpu_num, uint64_t vaddr)
{
    auto [ppage, fault] = vpage_to_ppage_map.insert({
        {cpu_num, vaddr >> LOG2_PAGE_SIZE},
        ppage_front()
    });

    // this vpage doesn't yet have a ppage mapping
    if (fault)
    {
        ppage_pop();

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
        output_statistics.virtual_page_count++;
#endif // PRINT_STATISTICS_INTO_FILE
    }

    auto paddr = champsim::splice_bits(ppage->second, vaddr, LOG2_PAGE_SIZE);
    if constexpr (champsim::debug_print)
    {
#if (USE_VCPKG == ENABLE)
        fmt::print("[VMEM] {} paddr: {:x} vaddr: {:x} fault: {}\n", __func__, paddr, vaddr, fault);
#endif // USE_VCPKG
    }

    return {paddr, fault ? minor_fault_penalty : 0};
}

std::pair<uint64_t, uint64_t> VirtualMemory::get_pte_pa(uint32_t cpu_num, uint64_t vaddr, std::size_t level)
{
    if (next_pte_page == 0)
    {
        next_pte_page = ppage_front();
        ppage_pop();
    }

    std::tuple key {cpu_num, vaddr >> shamt(level), level};
    auto [ppage, fault] = page_table.insert({key, next_pte_page});

    // this PTE doesn't yet have a mapping
    if (fault)
    {
        next_pte_page += pte_page_size;
        if (! (next_pte_page % PAGE_SIZE))
        {
            next_pte_page = ppage_front();
            ppage_pop();
        }

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
        output_statistics.valid_pte_count[level - 1]++;
#endif // PRINT_STATISTICS_INTO_FILE
    }

    auto offset = get_offset(vaddr, level);
    auto paddr  = champsim::splice_bits(ppage->second, offset * PTE_BYTES, champsim::lg2(pte_page_size));
    if constexpr (champsim::debug_print)
    {
#if (USE_VCPKG == ENABLE)
        fmt::print("[VMEM] {} paddr: {:x} vaddr: {:x} pt_page_offset: {} translation_level: {} fault: {}\n", __func__, paddr, vaddr, offset, level, fault);
#endif // USE_VCPKG
    }

    return {paddr, fault ? minor_fault_penalty : 0};
}

#endif // RAMULATOR

#else
/* Original code of ChampSim */

#include "ChampSim/dram_controller.h"

VirtualMemory::VirtualMemory(uint64_t page_table_page_size, std::size_t page_table_levels, uint64_t minor_penalty, MEMORY_CONTROLLER& dram)
: next_ppage(VMEM_RESERVE_CAPACITY), last_ppage(1ull << (LOG2_PAGE_SIZE + champsim::lg2(page_table_page_size / PTE_BYTES) * page_table_levels)),
  minor_fault_penalty(minor_penalty), pt_levels(page_table_levels), pte_page_size(page_table_page_size)
{
    assert(page_table_page_size > 1024);
    assert(page_table_page_size == (1ull << champsim::lg2(page_table_page_size)));
    assert(last_ppage > VMEM_RESERVE_CAPACITY);

    auto required_bits = champsim::lg2(last_ppage);
    if (required_bits > 64)
        fmt::print("WARNING: virtual memory configuration would require {} bits of addressing.\n", required_bits); // LCOV_EXCL_LINE
    if (required_bits > champsim::lg2(dram.size()))
        fmt::print("WARNING: physical memory size is smaller than virtual memory size.\n"); // LCOV_EXCL_LINE
}

uint64_t VirtualMemory::shamt(std::size_t level) const { return LOG2_PAGE_SIZE + champsim::lg2(pte_page_size / PTE_BYTES) * (level - 1); }

uint64_t VirtualMemory::get_offset(uint64_t vaddr, std::size_t level) const
{
    return (vaddr >> shamt(level)) & champsim::bitmask(champsim::lg2(pte_page_size / PTE_BYTES));
}

uint64_t VirtualMemory::ppage_front() const
{
    assert(available_ppages() > 0);
    return next_ppage;
}

void VirtualMemory::ppage_pop() { next_ppage += PAGE_SIZE; }

std::size_t VirtualMemory::available_ppages() const { return (last_ppage - next_ppage) / PAGE_SIZE; }

std::pair<uint64_t, uint64_t> VirtualMemory::va_to_pa(uint32_t cpu_num, uint64_t vaddr)
{
    auto [ppage, fault] = vpage_to_ppage_map.insert({
        {cpu_num, vaddr >> LOG2_PAGE_SIZE},
        ppage_front()
    });

    // this vpage doesn't yet have a ppage mapping
    if (fault)
        ppage_pop();

    auto paddr = champsim::splice_bits(ppage->second, vaddr, LOG2_PAGE_SIZE);
    if constexpr (champsim::debug_print)
    {
        fmt::print("[VMEM] {} paddr: {:x} vaddr: {:x} fault: {}\n", __func__, paddr, vaddr, fault);
    }

    return {paddr, fault ? minor_fault_penalty : 0};
}

std::pair<uint64_t, uint64_t> VirtualMemory::get_pte_pa(uint32_t cpu_num, uint64_t vaddr, std::size_t level)
{
    if (next_pte_page == 0)
    {
        next_pte_page = ppage_front();
        ppage_pop();
    }

    std::tuple key {cpu_num, vaddr >> shamt(level), level};
    auto [ppage, fault] = page_table.insert({key, next_pte_page});

    // this PTE doesn't yet have a mapping
    if (fault)
    {
        next_pte_page += pte_page_size;
        if (! (next_pte_page % PAGE_SIZE))
        {
            next_pte_page = ppage_front();
            ppage_pop();
        }
    }

    auto offset = get_offset(vaddr, level);
    auto paddr  = champsim::splice_bits(ppage->second, offset * PTE_BYTES, champsim::lg2(pte_page_size));
    if constexpr (champsim::debug_print)
    {
        fmt::print("[VMEM] {} paddr: {:x} vaddr: {:x} pt_page_offset: {} translation_level: {} fault: {}\n", __func__, paddr, vaddr, offset, level, fault);
    }

    return {paddr, fault ? minor_fault_penalty : 0};
}

#endif // USER_CODES
