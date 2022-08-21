#ifndef OPERABLE_H
#define OPERABLE_H

#include <iostream>
#include "ProjectConfiguration.h" // user file

namespace champsim
{

#if USER_CODES == ENABLE
// an abstract class, called operable, that is used only as a base class for inheritance.
#endif
class operable
{
public:
  const double CLOCK_SCALE;

  double leap_operation = 0;
  uint64_t current_cycle = 0;

  explicit operable(double scale) : CLOCK_SCALE(scale - 1) {}

  void _operate()
  {
#if RAMULATOR == ENABLE
#else
    // skip periodically
    if (leap_operation >= 1)
    {
      leap_operation -= 1;
      return;
    }
#endif

    operate();

#if RAMULATOR == ENABLE
#else
    leap_operation += CLOCK_SCALE;
#endif

    ++current_cycle;
  }

  virtual void operate() = 0;
  virtual void print_deadlock() {}
};

class by_next_operate
{
public:
  bool operator()(operable* lhs, operable* rhs) const { return lhs->leap_operation < rhs->leap_operation; }
};

} // namespace champsim

#endif
