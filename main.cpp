#include <iostream>
#include "tests.h"
#include "LR1_algorithm.h"

int main() {
  int successful_tests_number = 0;
  for (auto val : tests) {
    if (LR1_alg(val.rules, val.str_to_find) == val.is_contained) {
      ++successful_tests_number;
    }
  }
  if (successful_tests_number == tests.size()) {
    std::cout << "All tests ran successful";
    return 0;
  }
  std::cout << successful_tests_number << " of " << tests.size() << " ran successful";
}
