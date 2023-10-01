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

// #ifndef UTIL_H
// #define UTIL_H

// #include <cstdint>

// #include "ProjectConfiguration.h" // User file

// /** @note Calulate the bit mask for lower @b begin bits (start from 0 bit) and ignore the lower @b end bits.
//  *  @example
//  *  if begin = 3 and end = 0 are input, then (00...00111)(64 bit) is return.
//  *  if begin = 3 and end = 1 are input, then (00...00110)(64 bit) is return.
//  *  if begin = 3 and end = 2 are input, then (00...00100)(64 bit) is return.
//  *  if begin = 3 and end = 3 are input, then (00...00000)(64 bit) is return.
//  */
// constexpr uint64_t bitmask(std::size_t begin, std::size_t end = 0) { return ((1ull << (begin - end)) - 1) << end; }

// /** @note replace @b upper's lower @b bits bits with @b lower's lower @b bits bits.
//  *  @example
//  *  if upper = 0xb(1011), lower = 0x1(0001) and bits = 2, then (1001)(64 bit) is return.
//  */
// constexpr uint64_t splice_bits(uint64_t upper, uint64_t lower, std::size_t bits)
// {
//     return (upper & ~bitmask(bits)) | (lower & bitmask(bits));
// }

// /** @note get @b number's bits from @b begin bit to @b end bit.
//  *  @example
//  *  if number = 0xa(1010), begin = 1 and end = 0, then (0010)(64 bit) is return.
//  *  if number = 0xa(1010), begin = 4 and end = 1, then (0101)(64 bit) is return.
//  */
// inline uint64_t get_bits(uint64_t number, uint8_t begin, uint8_t end = 0)
// {
//     assert(begin >= end);
//     uint8_t length = begin - end + 1;
//     uint64_t mask  = ((1ull << length) - 1) << end;
//     return (number & mask) >> end;
// };

// inline uint64_t replace_bits(uint64_t upper, uint64_t lower, uint8_t begin, uint8_t end = 0)
// {
//     assert(begin >= end);
//     uint8_t length = begin - end + 1;
//     uint64_t mask  = ((1ull << length) - 1) << end;
//     return (upper & ~mask) | (lower & mask);
// };

// inline uint64_t clear_bits(uint64_t number, uint8_t begin, uint8_t end = 0)
// {
//     assert(begin >= end);
//     uint8_t length = begin - end + 1;
//     uint64_t mask  = ~(((1ull << length) - 1) << end);
//     return number & mask;
// };

// inline uint64_t set_bits(uint64_t number, uint8_t begin, uint8_t end = 0)
// {
//     assert(begin >= end);
//     uint8_t length = begin - end + 1;
//     uint64_t mask  = ((1ull << length) - 1) << end;
//     return number | mask;
// };

// #endif
