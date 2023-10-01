#ifndef __RAMULATOR_H__
#define __RAMULATOR_H__

#include <deque>
#include <map>
#include <tuple>

#include "Ramulator/Config.h"

/*
namespace ramulator
{
class Request;
class Gem5Wrapper;
}

class Ramulator : public AbstractMemory
{
private:

    class TestPort : public SlavePort
    {
    private:
        Ramulator& mem;
    public:
        TestPort(const std::string& _name, Ramulator& _mem) : SlavePort(_name, &_mem), mem(_mem) {}
    protected:
        Tick recvAtomic(PacketPtr pkt)
        {
            // modified to perform a fixed latency return for atomic packets to enable fast forwarding
            // assert(false && "only accepts functional or timing packets");
            return mem.recvAtomic(pkt);
        }

        void recvFunctional(PacketPtr pkt)
        {
            mem.recvFunctional(pkt);
        }

        bool recvTimingReq(PacketPtr pkt)
        {
            return mem.recvTimingReq(pkt);
        }

        void recvRetry()
        {
            mem.recvRetry();
        }

        AddrRangeList getAddrRanges() const
        {
            AddrRangeList ranges;
            ranges.push_back(mem.getAddrRange());
            return ranges;
        }
    } port;

    unsigned int requestsInFlight;
    std::map<long, std::deque<PacketPtr> > reads;
    std::map<long, std::deque<PacketPtr> > writes;
    std::deque<PacketPtr> resp_queue;
    std::deque<PacketPtr> pending_del;
    DrainManager* drain_manager;

    std::string config_file;
    ramulator::Config configs;
    ramulator::Gem5Wrapper* wrapper;
    std::function<void(ramulator::Request&)> read_cb_func;
    std::function<void(ramulator::Request&)> write_cb_func;
    Tick ticks_per_clk;
    bool resp_stall;
    bool req_stall;

    unsigned int numOutstanding() const { return requestsInFlight + resp_queue.size(); }

    void sendResponse();
    void tick();

    EventWrapper<Ramulator, &Ramulator::sendResponse> send_resp_event;
    EventWrapper<Ramulator, &Ramulator::tick> tick_event;

public:
    typedef RamulatorParams Params;
    Ramulator(const Params* p);
    virtual void init();
    virtual void startup();
    unsigned int drain(DrainManager* dm);
    virtual BaseSlavePort& getSlavePort(const std::string& if_name,
                                        PortID idx = InvalidPortID);
    ~Ramulator();
protected:
    Tick recvAtomic(PacketPtr pkt);
    void recvFunctional(PacketPtr pkt);
    bool recvTimingReq(PacketPtr pkt);
    void recvRetry();
    void accessAndRespond(PacketPtr pkt);
    void readComplete(ramulator::Request& req);
    void writeComplete(ramulator::Request& req);
};
*/

#endif // __RAMULATOR_H__