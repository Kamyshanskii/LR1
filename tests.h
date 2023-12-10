#include "LR1_algorithm.h"

struct test {
  const std::vector<std::string> rules;
  const std::string str_to_find;
  const bool is_contained;
};

std::vector<test> tests = {{{"S->AB", "A->a", "B->b"}, "ab", true},
                           {{"S->AB", "S->", "A->a", "B->b"}, "", true},
                           {{"S->AB", "A->BC", "A->a", "B->b", "C->c"}, "abc", false},
                           {{"S->AB", "S->", "A->a", "B->b", "B->AA"}, "aaaa",false}};
