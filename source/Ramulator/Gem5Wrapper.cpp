#include "Ramulator/Gem5Wrapper.h"

#include <map>

#include "Ramulator/Config.h"
#include "Ramulator/DDR3.h"
#include "Ramulator/DDR4.h"
#include "Ramulator/GDDR5.h"
#include "Ramulator/HBM.h"
#include "Ramulator/LPDDR3.h"
#include "Ramulator/LPDDR4.h"
#include "Ramulator/Memory.h"
#include "Ramulator/MemoryFactory.h"
#include "Ramulator/Request.h"
#include "Ramulator/SALP.h"
#include "Ramulator/WideIO.h"
#include "Ramulator/WideIO2.h"

using namespace ramulator;

static map<string, function<MemoryBase*(const Config&, int)> > name_to_func = {
    {     "DDR3",    &MemoryFactory<DDR3>::create},
    {     "DDR4",    &MemoryFactory<DDR4>::create},
    {   "LPDDR3",  &MemoryFactory<LPDDR3>::create},
    {   "LPDDR4",  &MemoryFactory<LPDDR4>::create},
    {    "GDDR5",   &MemoryFactory<GDDR5>::create},
    {   "WideIO",  &MemoryFactory<WideIO>::create},
    {  "WideIO2", &MemoryFactory<WideIO2>::create},
    {      "HBM",     &MemoryFactory<HBM>::create},
    {   "SALP-1",    &MemoryFactory<SALP>::create},
    {   "SALP-2",    &MemoryFactory<SALP>::create},
    {"SALP-MASA",    &MemoryFactory<SALP>::create},
};

Gem5Wrapper::Gem5Wrapper(const Config& configs, int cacheline)
{
    const string& std_name = configs["standard"];
    assert(name_to_func.find(std_name) != name_to_func.end() && "unrecognized standard name");
    mem = name_to_func[std_name](configs, cacheline);
    tCK = mem->clk_ns();
}

Gem5Wrapper::~Gem5Wrapper()
{
    delete mem;
}

void Gem5Wrapper::tick()
{
    mem->tick();
}

bool Gem5Wrapper::send(Request req)
{
    return mem->send(req);
}

void Gem5Wrapper::finish(void)
{
    mem->finish();
}
