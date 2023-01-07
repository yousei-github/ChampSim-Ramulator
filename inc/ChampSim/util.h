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

#ifndef UTIL_H
#define UTIL_H

#include <cstdint>
#include "ProjectConfiguration.h" // user file

constexpr unsigned lg2(uint64_t n) { return n < 2 ? 0 : 1 + lg2(n / 2); }

#if (USER_CODES == ENABLE)
/** @note Calulate the bit mask for lower @b begin bits (start from 0 bit) and ignore the lower @b end bits.
 *  @example
 *  if begin = 3 and end = 0 are input, then (00...00111)(64 bit) is return.
 *  if begin = 3 and end = 1 are input, then (00...00110)(64 bit) is return.
 *  if begin = 3 and end = 2 are input, then (00...00100)(64 bit) is return.
 *  if begin = 3 and end = 3 are input, then (00...00000)(64 bit) is return.
 */
#endif
constexpr uint64_t bitmask(std::size_t begin, std::size_t end = 0) { return ((1ull << (begin - end)) - 1) << end; }

#if (USER_CODES == ENABLE)
/** @note replace @b upper's lower @b bits bits with @b lower's lower @b bits bits.
 *  @example
 *  if upper = 0xb(1011), lower = 0x1(0001) and bits = 2, then (1001)(64 bit) is return.
 */
#endif
constexpr uint64_t splice_bits(uint64_t upper, uint64_t lower, std::size_t bits) { return (upper & ~bitmask(bits)) | (lower & bitmask(bits)); }

#if (USER_CODES == ENABLE)
/** @note get @b number's bits from @b begin bit to @b end bit.
 *  @example
 *  if number = 0xa(1010), begin = 1 and end = 0, then (0010)(64 bit) is return.
 *  if number = 0xa(1010), begin = 4 and end = 1, then (0101)(64 bit) is return.
 */
inline uint64_t get_bits(uint64_t number, uint8_t begin, uint8_t end = 0)
{
  assert(begin >= end);
  uint8_t length = begin - end + 1;
  uint64_t mask = ((1ull << length) - 1) << end;
  return (number & mask) >> end;
};

inline uint64_t replace_bits(uint64_t upper, uint64_t lower, uint8_t begin, uint8_t end = 0)
{
  assert(begin >= end);
  uint8_t length = begin - end + 1;
  uint64_t mask = ((1ull << length) - 1) << end;
  return (upper & ~mask) | (lower & mask);
};

inline uint64_t clear_bits(uint64_t number, uint8_t begin, uint8_t end = 0)
{
  assert(begin >= end);
  uint8_t length = begin - end + 1;
  uint64_t mask = ~(((1ull << length) - 1) << end);
  return number & mask;
};

inline uint64_t set_bits(uint64_t number, uint8_t begin, uint8_t end = 0)
{
  assert(begin >= end);
  uint8_t length = begin - end + 1;
  uint64_t mask = ((1ull << length) - 1) << end;
  return number | mask;
};

#endif  // USER_CODES

template <typename T>
struct is_valid
{
  using argument_type = T;
  is_valid() {}
  bool operator()(const argument_type& test) { return test.valid; }
};

template <typename T>
struct eq_addr
{
  using argument_type = T;
  const decltype(argument_type::address) val;
  const std::size_t shamt = 0;

  explicit eq_addr(decltype(argument_type::address) val) : val(val) {}
  eq_addr(decltype(argument_type::address) val, std::size_t shamt) : val(val), shamt(shamt) {}

  bool operator()(const argument_type& test)
  {
    is_valid<argument_type> validtest;
    return validtest(test) && (test.address >> shamt) == (val >> shamt);
  }
};

template <typename T, typename BIN, typename U = T, typename UN_T = is_valid<T>, typename UN_U = is_valid<U>>
struct invalid_is_minimal
{
  bool operator()(const T& lhs, const U& rhs)
  {
    UN_T lhs_unary;
    UN_U rhs_unary;
    BIN cmp;

    return !lhs_unary(lhs) || (rhs_unary(rhs) && cmp(lhs, rhs));
  }
};

template <typename T, typename BIN, typename U = T, typename UN_T = is_valid<T>, typename UN_U = is_valid<U>>
struct invalid_is_maximal
{
  bool operator()(const T& lhs, const U& rhs)
  {
    UN_T lhs_unary;
    UN_U rhs_unary;
    BIN cmp;

    return !rhs_unary(rhs) || (lhs_unary(lhs) && cmp(lhs, rhs));
  }
};

template <typename T, typename U = T>
struct cmp_event_cycle
{
  bool operator()(const T& lhs, const U& rhs) { return lhs.event_cycle < rhs.event_cycle; }
};

template <typename T>
struct min_event_cycle : invalid_is_maximal<T, cmp_event_cycle<T>>
{
};

template <typename T, typename U = T>
struct cmp_lru
{
  bool operator()(const T& lhs, const U& rhs) { return lhs.lru < rhs.lru; }
};

/*
 * A comparator to determine the LRU element. To use this comparator, the type
 * must have a member variable named "lru" and have a specialization of
 * is_valid<>.
 *
 * To use:
 *     auto lru_elem = std::max_element(std::begin(set), std::end(set),
 * lru_comparator<BLOCK>());
 *
 * The MRU element can be found using std::min_element instead.
 */
template <typename T, typename U = T>
struct lru_comparator : invalid_is_maximal<T, cmp_lru<T, U>, U>
{
  using first_argument_type = T;
  using second_argument_type = U;
};

/*
 * A functor to reorder elements to a new LRU order.
 * The type must have a member variable named "lru".
 *
 * To use:
 *     std::for_each(std::begin(set), std::end(set),
 * lru_updater<BLOCK>(hit_element));
 */
template <typename T>
struct lru_updater
{
  const decltype(T::lru) val;
  explicit lru_updater(decltype(T::lru) val) : val(val) {}

  template <typename U>
  explicit lru_updater(U iter) : val(iter->lru)
  {
  }

  void operator()(T& x)
  {
    if (x.lru == val)
      x.lru = 0;
    else
      ++x.lru;
  }
};

template <typename T, typename U = T>
struct ord_event_cycle
{
  using first_argument_type = T;
  using second_argument_type = U;
  bool operator()(const first_argument_type& lhs, const second_argument_type& rhs)
  {
    is_valid<first_argument_type> first_validtest;
    is_valid<second_argument_type> second_validtest;
    return !second_validtest(rhs) || (first_validtest(lhs) && lhs.event_cycle < rhs.event_cycle);
  }
};

#endif
