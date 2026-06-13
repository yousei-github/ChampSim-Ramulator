#include "Ramulator2/base/request.h"

namespace Ramulator
{

#if (USER_CODES == ENABLE)

Request::Request(Addr_t addr, int type): addr(addr), type_id(type) {};

Request::Request(AddrVec_t addr_vec, int type): addr_vec(addr_vec), type_id(type) {};

Request::Request(Addr_t addr, int type, int source_id, std::function<void(Request&)> callback): addr(addr), type_id(type), source_id(source_id), callback(callback) {};

Request::Request(Addr_t addr, int type, int source_id, std::function<void(Request&)> callback, uint8_t memory_id)
: addr(addr), type_id(type), source_id(source_id), callback(callback), memory_id(memory_id) {};

#if (RAMULATOR2 == ENABLE)

Request::Request(Addr_t addr, int type, int source_id, std::function<void(Request&)> callback, DRAM_CHANNEL::request_type packet, uint8_t memory_id)
: addr(addr), type_id(type), source_id(source_id), callback(callback), packet(packet), memory_id(memory_id) {};

#endif /* RAMULATOR */

#else
/* Original code of Ramulator */

Request::Request(Addr_t addr, int type): addr(addr), type_id(type) {};

Request::Request(AddrVec_t addr_vec, int type): addr_vec(addr_vec), type_id(type) {};

Request::Request(Addr_t addr, int type, int source_id, std::function<void(Request&)> callback): addr(addr), type_id(type), source_id(source_id), callback(callback) {};

#endif /* USER_CODES */

} // namespace Ramulator
