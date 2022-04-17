#ifndef AUTOMATA_MAX_MATCHING_PREFIX_H
#define AUTOMATA_MAX_MATCHING_PREFIX_H

#include "automaton.h"
#include "doctest.h"
#include <queue>

class MaxMatchingPrefixFinder {
public:
  MaxMatchingPrefixFinder(const automata::NondeterministicAutomaton &automaton, std::string_view pattern);

  static std::size_t GetMaxMatchingPrefix(const regex::Regex &regex, const std::string &pattern);

private:
  const automata::NondeterministicAutomaton &automaton_;
  std::string_view pattern_;
  std::vector<std::vector<bool>> is_possible_prefix_;
  std::queue<std::pair<std::size_t, std::size_t>> to_process_;
  std::size_t max_matching_prefix_ = 0;

  std::size_t ComputeMaxMatchingPrefix();

  void ProcessState(size_t state, size_t prefix_length);

  void ProcessTransition(size_t prefix_length, const automata::Transition<std::string> &transition);

  static char GetSingleTransitionSymbol(const automata::Transition<std::string> &transition);

  TEST_CASE_CLASS("Compute max matching prefix for regex") {
    SUBCASE("") {
      CHECK_EQ(5, MaxMatchingPrefixFinder::GetMaxMatchingPrefix(regex::Regex::Parse("(a*b)*"), "aababac"));
    }
    SUBCASE("") {
      CHECK_EQ(0, MaxMatchingPrefixFinder::GetMaxMatchingPrefix(regex::Regex::Parse("c(a+b)"), "aa"));
    }
    SUBCASE("") {
      CHECK_EQ(2, MaxMatchingPrefixFinder::GetMaxMatchingPrefix(regex::Regex::Parse("c(a+b)"), "cb"));
    }
  }
  SCENARIO_CLASS("Compute max matching prefix for automaton") {
    GIVEN("automaton that accepts one string") {
      automata::NondeterministicAutomaton automaton{3, 0, {2}, {{0, 1, "a"}, {1, 2, "b"}}};
      WHEN("the pattern is empty") {
        THEN("returns 0") {
          CHECK_EQ(0, MaxMatchingPrefixFinder(automaton, "").ComputeMaxMatchingPrefix());
        }
      }
      WHEN("the pattern is prefix of the accepted string") {
        THEN("returns 0") {
          CHECK_EQ(0, MaxMatchingPrefixFinder(automaton, "a").ComputeMaxMatchingPrefix());
        }
      }
      WHEN("the accepted string is prefix of the pattern") {
        THEN("returns length of accepted string") {
          CHECK_EQ(2, MaxMatchingPrefixFinder(automaton, "abcd").ComputeMaxMatchingPrefix());
        }
      }
    }
    GIVEN("automaton that accepts strings of letter a") {
      automata::NondeterministicAutomaton automaton{1, 0, {0}, {{0, 0, "a"}}};
      WHEN("the pattern starts with n as") {
        THEN("returns n") {
          CHECK_EQ(3, MaxMatchingPrefixFinder(automaton, "aaabc").ComputeMaxMatchingPrefix());
        }
      }
    }
    GIVEN("automaton that accepts strings of length n") {
      automata::NondeterministicAutomaton automaton{3, 0, {2}, {{0, 1, "a"}, {0, 1, "b"}, {1, 2, "a"}, {1, 2, "b"}}};
      WHEN("pattern length is less than n") {
        THEN("returns 0") {
          CHECK_EQ(0, MaxMatchingPrefixFinder(automaton, "a").ComputeMaxMatchingPrefix());
        }
      }
      WHEN("pattern length is at least n") {
        THEN("returns n") {
          CHECK_EQ(2, MaxMatchingPrefixFinder(automaton, "baa").ComputeMaxMatchingPrefix());
        }
      }
    }
  }

  SCENARIO_CLASS("Process transition") {
    automata::NondeterministicAutomaton automaton{3, 2, {1}, {{2, 0, "a"}, {2, 1, "b"}}};
    MaxMatchingPrefixFinder finder(automaton, "ab");

    SUBCASE("Process state") {
      WHEN("current state is accepting") {
        finder.ProcessState(1, 2);
        THEN("max prefix length is updated") {
          CHECK_EQ(2, finder.max_matching_prefix_);
        }
      }
      WHEN("Current state is not accepting") {
        finder.ProcessState(2, 2);
        THEN("max prefix length is not updated") {
          CHECK_EQ(0, finder.max_matching_prefix_);
        }
      }
      WHEN("current length is maximal possible") {
        THEN("nothing is done") {
          finder.ProcessState(0, 2);
        }
      }
      WHEN("state is processed") {
        finder.ProcessState(2, 0);
        THEN("reachable states are marked as possible") {
          CHECK(finder.is_possible_prefix_[0][1]);
          CHECK_FALSE(finder.is_possible_prefix_[1][1]);
        }
      }
    }

    SUBCASE("Process transition") {
      WHEN("symbol is same as in pattern and state is not marked yet") {
        finder.ProcessTransition(0, {"a", 0});
        THEN("new state is added") {
          CHECK(finder.is_possible_prefix_[0][1]);
          CHECK_EQ(std::pair<std::size_t, std::size_t>{0, 1}, finder.to_process_.front());
        }
      }
      WHEN("next symbol in pattern is different") {
        finder.ProcessTransition(0, {"b", 0});
        THEN("new state is not added") {
          CHECK_FALSE(finder.is_possible_prefix_[0][1]);
          CHECK(finder.to_process_.empty());
        }
      }
      WHEN("state is already marked as possible") {
        finder.is_possible_prefix_[0][1] = true;
        finder.ProcessTransition(0, {"b", 0});
        THEN("new state is not added") {
          CHECK(finder.to_process_.empty());
        }
      }
    }
  }

  TEST_CASE_CLASS("Get single transition symbol") {
    SUBCASE("Single symbol") {
      CHECK_EQ('a', MaxMatchingPrefixFinder::GetSingleTransitionSymbol(automata::Transition<std::string>{"a", 0}));
    }
    SUBCASE("Empty string") {
      CHECK_THROWS_AS(MaxMatchingPrefixFinder::GetSingleTransitionSymbol(automata::Transition<std::string>{"", 0}),
                      automata::BadAutomatonException);
    }
    SUBCASE("Long string") {
      CHECK_THROWS_AS(MaxMatchingPrefixFinder::GetSingleTransitionSymbol(automata::Transition<std::string>{"ab", 0}),
                      automata::BadAutomatonException);
    }
  }
};

#endif //AUTOMATA_MAX_MATCHING_PREFIX_H
