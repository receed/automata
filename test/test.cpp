#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.h"
#include "automaton.h"

TEST_SUITE("Split transitions") {
  TEST_CASE("Short transitions") {
    CHECK_EQ(
        NondeterministicAutomaton{3, 0, {{0, 1, ""}, {1, 2, "a"}}, {1}}.SplitTransitions(),
        NondeterministicAutomaton{3, 0, {{0, 1, ""}, {1, 2, "a"}}, {1}}
    );
  }
  TEST_CASE("Long transitions") {
    CHECK_EQ(
        NondeterministicAutomaton{3, 0, {{0, 2, "abcd"}, {0, 1, "xy"}}, {1}}.SplitTransitions(),
        NondeterministicAutomaton{7, 0, {{0, 3, "a"}, {3, 4, "b"}, {4, 5, "c"}, {5, 2, "d"}, {0, 6, "x"}, {6, 1, "y"}}, {1}}
    );
  }
}

TEST_CASE("Remove empty transitions") {
  CHECK_EQ(
      NondeterministicAutomaton{5, 0, {{1, 0, ""}, {2, 1, ""}, {3, 2, ""}, {1, 4, "ab"}}, {1}}.RemoveEmptyTransitions(),
      NondeterministicAutomaton{5, 0, {{1, 0, ""}, {1, 4, "ab"}, {2, 0, ""}, {2, 1, ""}, {2, 4, "ab"}, {3, 0, ""}, {3, 1, ""}, {3, 2, ""}, {3, 4, "ab"}}, {1, 2, 3}}
  );
}