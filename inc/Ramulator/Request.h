#ifndef __REQUEST_H
#define __REQUEST_H

#include <vector>
#include <functional>
#include "ProjectConfiguration.h" // user file
#if (USER_CODES) == (ENABLE)
#include "memory_class.h"
#include "block.h"
#include "champsim_constants.h"
#include <array>
#endif

using namespace std;

namespace ramulator
{

class Request
{
public:
    bool is_first_command;
    long addr;
    // long addr_row;
    vector<int> addr_vec;
    // specify which core this request sent from, for virtual address translation
    int coreid;

    enum class Type
    {
        READ,
        WRITE,
        REFRESH,
        POWERDOWN,
        SELFREFRESH,
        EXTENSION,
        MAX
    } type;

    long arrive = -1;
    long depart = -1;
    function<void(Request&)> callback; // call back with more info

#if (USER_CODES == ENABLE) && (RAMULATOR == ENABLE)
    PACKET packet;
    void receive(Request& request)
    {
        // Note here no data will be send back to cpu.
        for (auto ret : request.packet.to_return)
            ret->return_data(&(request.packet));
    };
    
    std::array<uint8_t, BLOCK_SIZE> data = {0}; // a cache line
#endif

    Request(long addr, Type type, int coreid = 0)
        : is_first_command(true), addr(addr), coreid(coreid), type(type),
        callback([](Request& req) {})
    {
    }

    Request(long addr, Type type, function<void(Request&)> callback, int coreid = 0)
        : is_first_command(true), addr(addr), coreid(coreid), type(type), callback(callback)
    {
    }

#if (USER_CODES == ENABLE) && (RAMULATOR == ENABLE)
    Request(long addr, Type type, PACKET packet, int coreid = 0)
        : is_first_command(true), addr(addr), coreid(coreid), type(type), packet(packet)
    {
        callback = std::bind(&Request::receive, this, placeholders::_1);
    }

    Request(long addr, Type type, function<void(Request&)> callback, PACKET packet, int coreid = 0)
        : is_first_command(true), addr(addr), coreid(coreid), type(type), callback(callback), packet(packet)
    {
    }
#endif

    Request(vector<int>& addr_vec, Type type, function<void(Request&)> callback, int coreid = 0)
        : is_first_command(true), addr_vec(addr_vec), coreid(coreid), type(type), callback(callback)
    {
    }

    Request()
        : is_first_command(true), coreid(0)
    {
    }
};

} /*namespace ramulator*/

#endif /*__REQUEST_H*/

