// #include "base/callback.hh"
// #include "mem/ramulator.hh"
// #include "Ramulator/src/Gem5Wrapper.h"
// #include "Ramulator/src/Request.h"
// #include "sim/system.hh"
// #include "debug/Ramulator.hh"
#include "Ramulator/Ramulator.h"

#include "Ramulator/Gem5Wrapper.h"
#include "Ramulator/Request.h"

/*
Ramulator::Ramulator(const Params* p) :
    AbstractMemory(p),
    port(name() + ".port", *this),
    requestsInFlight(0),
    drain_manager(NULL),
    config_file(p->config_file),
    configs(p->config_file),
    wrapper(NULL),
    read_cb_func(std::bind(&Ramulator::readComplete, this, std::placeholders::_1)),
    write_cb_func(std::bind(&Ramulator::writeComplete, this, std::placeholders::_1)),
    ticks_per_clk(0),
    resp_stall(false),
    req_stall(false),
    send_resp_event(this),
    tick_event(this)
{
    configs.set_core_num(p->num_cpus);
}
Ramulator::~Ramulator()
{
    delete wrapper;
}

void Ramulator::init()
{
    if (!port.isConnected())
    {
        fatal("Ramulator port not connected\n");
    }
    else
    {
        port.sendRangeChange();
    }
    wrapper = new ramulator::Gem5Wrapper(configs, system()->cacheLineSize());
    ticks_per_clk = Tick(wrapper->tCK * SimClock::Float::ns);

    DPRINTF(Ramulator, "Instantiated Ramulator with config file '%s' (tCK=%lf, %d ticks per clk)\n",
            config_file.c_str(), wrapper->tCK, ticks_per_clk);
    Callback* cb = new MakeCallback<ramulator::Gem5Wrapper, &ramulator::Gem5Wrapper::finish>(wrapper);
    registerExitCallback(cb);
}

void Ramulator::startup()
{
    schedule(tick_event, clockEdge());
}

unsigned int Ramulator::drain(DrainManager* dm)
{
    DPRINTF(Ramulator, "Requested to drain\n");
    // updated to include all in-flight requests
    // if (resp_queue.size()) {
    if (numOutstanding())
    {
        setDrainState(Drainable::Draining);
        drain_manager = dm;
        return 1;
    }
    else
    {
        setDrainState(Drainable::Drained);
        return 0;
    }
}

BaseSlavePort& Ramulator::getSlavePort(const std::string& if_name, PortID idx)
{
    if (if_name != "port")
    {
        return MemObject::getSlavePort(if_name, idx);
    }
    else
    {
        return port;
    }
}

void Ramulator::sendResponse()
{
    assert(!resp_stall);
    assert(!resp_queue.empty());

    DPRINTF(Ramulator, "Attempting to send response\n");

    long addr = resp_queue.front()->getAddr();
    if (port.sendTimingResp(resp_queue.front()))
    {
        DPRINTF(Ramulator, "Response to %ld sent.\n", addr);
        resp_queue.pop_front();
        if (resp_queue.size() && !send_resp_event.scheduled())
            schedule(send_resp_event, curTick());

        // check if we were asked to drain and if we are now done
        if (drain_manager && numOutstanding() == 0)
        {
            drain_manager->signalDrainDone();
            drain_manager = NULL;
        }
    }
    else
        resp_stall = true;
}

void Ramulator::tick()
{
    wrapper->tick();
    if (req_stall)
    {
        req_stall = false;
        port.sendRetry();
    }
    schedule(tick_event, curTick() + ticks_per_clk);
}

// added an atomic packet response function to enable fast forwarding
Tick Ramulator::recvAtomic(PacketPtr pkt)
{
    access(pkt);

    // set an fixed arbitrary 50ns response time for atomic requests
    return pkt->memInhibitAsserted() ? 0 : 50000;
}

void Ramulator::recvFunctional(PacketPtr pkt)
{
    pkt->pushLabel(name());
    functionalAccess(pkt);
    for (auto i = resp_queue.begin(); i != resp_queue.end(); i)
        pkt->checkFunctional(*i);
    pkt->popLabel();
}

bool Ramulator::recvTimingReq(PacketPtr pkt)
{
    // we should never see a new request while in retry
    assert(!req_stall);

    for (PacketPtr pendPkt : pending_del)
        delete pendPkt;
    pending_del.clear();

    if (pkt->memInhibitAsserted())
    {
        // snooper will supply based on copy of packet
        // still target's responsibility to delete packet
        pending_del.push_back(pkt);
        return true;
    }

    bool accepted = true;
    if (pkt->isRead())
    {
        DPRINTF(Ramulator, "context id: %d, thread id: %d\n", pkt->req->contextId(),
                pkt->req->threadId());
        ramulator::Request req(pkt->getAddr(), ramulator::Request::Type::READ, read_cb_func, pkt->req->contextId());
        accepted = wrapper->send(req);
        if (accepted)
        {
            reads[req.addr].push_back(pkt);
            DPRINTF(Ramulator, "Read to %ld accepted.\n", req.addr);

            // added counter to track requests in flight
            ++requestsInFlight;
        }
        else
        {
            req_stall = true;
        }
    }
    else if (pkt->isWrite())
    {
        // Detailed CPU model always comes along with cache model enabled and
        // write requests are caused by cache eviction, so it shouldn't be
        // tallied for any core/thread
        ramulator::Request req(pkt->getAddr(), ramulator::Request::Type::WRITE, write_cb_func, 0);
        accepted = wrapper->send(req);
        if (accepted)
        {
            accessAndRespond(pkt);
            DPRINTF(Ramulator, "Write to %ld accepted and served.\n", req.addr);

            // added counter to track requests in flight
            ++requestsInFlight;
        }
        else
        {
            req_stall = true;
        }
    }
    else
    {
        // keep it simple and just respond if necessary
        accessAndRespond(pkt);
    }
    return accepted;
}

void Ramulator::recvRetry()
{
    DPRINTF(Ramulator, "Retrying\n");

    assert(resp_stall);
    resp_stall = false;
    sendResponse();
}

void Ramulator::accessAndRespond(PacketPtr pkt)
{
    bool need_resp = pkt->needsResponse();
    access(pkt);
    if (need_resp)
    {
        assert(pkt->isResponse());
        pkt->busFirstWordDelay = pkt->busLastWordDelay = 0;

        DPRINTF(Ramulator, "Queuing response for address %lld\n",
                pkt->getAddr());

        resp_queue.push_back(pkt);
        if (!resp_stall && !send_resp_event.scheduled())
            schedule(send_resp_event, curTick());
    }
    else
        pending_del.push_back(pkt);
}

void Ramulator::readComplete(ramulator::Request& req)
{
    DPRINTF(Ramulator, "Read to %ld completed.\n", req.addr);
    auto& pkt_q = reads.find(req.addr)->second;
    PacketPtr pkt = pkt_q.front();
    pkt_q.pop_front();
    if (!pkt_q.size())
        reads.erase(req.addr);

    // added counter to track requests in flight
    --requestsInFlight;

    accessAndRespond(pkt);
}

void Ramulator::writeComplete(ramulator::Request& req)
{
    DPRINTF(Ramulator, "Write to %ld completed.\n", req.addr);

    // added counter to track requests in flight
    --requestsInFlight;

    // check if we were asked to drain and if we are now done
    if (drain_manager && numOutstanding() == 0)
    {
        drain_manager->signalDrainDone();
        drain_manager = NULL;
    }
}

Ramulator* RamulatorParams::create()
{
    return new Ramulator(this);
}
*/
