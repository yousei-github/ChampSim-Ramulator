#ifndef __REQUEST_H
#define __REQUEST_H

#include <functional>
#include <vector>

#include "ProjectConfiguration.h" // User file

#if (USER_CODES == ENABLE)
#include <array>

#include "ChampSim/champsim_constants.h"
#include "ChampSim/dram_controller.h"

#endif // USER_CODES

using namespace std;

namespace ramulator
{

class Request
{
public:
#if (USER_CODES == ENABLE) && (RAMULATOR == ENABLE)
    // bool is_first_command;
    // long addr;
    // // long addr_row;
    // vector<int> addr_vec;
    // // specify which core this request sent from, for virtual address translation
    // int coreid;

    // enum class Type
    // {
    //     READ,
    //     WRITE,
    //     REFRESH,
    //     POWERDOWN,
    //     SELFREFRESH,
    //     EXTENSION,
    //     MAX
    // } type;

    // long arrive = -1;
    // long depart = -1;
    // function<void(Request&)> callback; // call back with more info

    // Request(long addr, Type type, int coreid = 0)
    // : is_first_command(true), addr(addr), coreid(coreid), type(type),
    //   callback([](Request& req) {})
    // {
    // }

    // Request(long addr, Type type, function<void(Request&)> callback, int coreid = 0)
    // : is_first_command(true), addr(addr), coreid(coreid), type(type), callback(callback)
    // {
    // }

    // Request(vector<int>& addr_vec, Type type, function<void(Request&)> callback, int coreid = 0)
    // : is_first_command(true), addr_vec(addr_vec), coreid(coreid), type(type), callback(callback)
    // {
    // }

    // Request()
    // : is_first_command(true), coreid(0)
    // {
    // }

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

    // ChampSim's memory controller's memory request
    DRAM_CHANNEL::request_type packet;

    std::array<uint8_t, BLOCK_SIZE> data = {0}; // A cache line
    uint8_t memory_id                    = NUMBER_OF_MEMORIES;

    /* Member functions */
    Request(long addr, Type type, int coreid = 0)
    : is_first_command(true), addr(addr), coreid(coreid), type(type),
      callback([](Request& req) {})
    {
    }

    Request(long addr, Type type, function<void(Request&)> callback, int coreid = 0)
    : is_first_command(true), addr(addr), coreid(coreid), type(type), callback(callback)
    {
    }

    Request(long addr, Type type, function<void(Request&)> callback, int coreid, uint8_t memory_id)
    : is_first_command(true), addr(addr), coreid(coreid), type(type), callback(callback), memory_id(memory_id)
    {
    }

    // This instructor is used for ChampSim's memory controller
    Request(long addr, Type type, function<void(Request&)> callback, DRAM_CHANNEL::request_type packet, int coreid, uint8_t memory_id)
    : is_first_command(true), addr(addr), coreid(coreid), type(type), callback(callback), packet(packet), memory_id(memory_id)
    {
    }

    Request(vector<int>& addr_vec, Type type, function<void(Request&)> callback, int coreid = 0)
    : is_first_command(true), addr_vec(addr_vec), coreid(coreid), type(type), callback(callback)
    {
    }

    Request()
    : is_first_command(true), coreid(0)
    {
    }
#else
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

    Request(long addr, Type type, int coreid = 0)
    : is_first_command(true), addr(addr), coreid(coreid), type(type),
      callback([](Request& req) {})
    {
    }

    Request(long addr, Type type, function<void(Request&)> callback, int coreid = 0)
    : is_first_command(true), addr(addr), coreid(coreid), type(type), callback(callback)
    {
    }

    Request(vector<int>& addr_vec, Type type, function<void(Request&)> callback, int coreid = 0)
    : is_first_command(true), addr_vec(addr_vec), coreid(coreid), type(type), callback(callback)
    {
    }

    Request()
    : is_first_command(true), coreid(0)
    {
    }
#endif // USER_CODES, RAMULATOR
};

} /*namespace ramulator*/

#endif /*__REQUEST_H*/
