#include <iostream>
#include <fstream>
#include <map>
#include <list>

#include "Ramulator2/example/example_ifce.h"
#include "Ramulator2/base/serialization.h"

namespace Ramulator {

class ComplicatedImpl : public ExampleIfce, public Implementation {
  RAMULATOR_REGISTER_IMPLEMENTATION(ExampleIfce, ComplicatedImpl, "ComplicatedImpl", "An example of a complicated implementation class.");
  public:
    void init() {};
    void special_function();
}; 

}        // namespace Ramulator