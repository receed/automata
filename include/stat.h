#ifndef AUTOMATA_STAT_H
#define AUTOMATA_STAT_H

#include "automaton.h"
#include <queue>

namespace stat {
  std::size_t GetMaxMatchingPrefix(const automata::NondeterministicAutomaton &automaton, const std::string &pattern) {
    auto is_possible_prefix = std::vector(automaton.GetStateNumber(), std::vector(pattern.size() + 1, 0));
    is_possible_prefix[automaton.initial_state()][0] = true;
    std::queue<std::pair<std::size_t, std::size_t>> to_process;
    to_process.emplace(automaton.initial_state(), 0);
    while (!to_process.empty()) {
      auto[state, prefix_length] = to_process.front();
      to_process.pop();
      if (prefix_length == pattern.size()) {
        continue;
      }
      for (const auto &transition: automaton.GetTransitions(state)) {
        if (transition.symbol.size() != 1) {
          throw automata::BadAutomatonException("Symbol of length not equal to 1");
        }
        if (transition.symbol[0] == pattern[prefix_length] &&
            !is_possible_prefix[transition.to_state][prefix_length + 1]) {
          is_possible_prefix[transition.to_state][prefix_length + 1] = true;
          to_process.emplace(transition.to_state, prefix_length + 1);
        }
      }
    }
    std::size_t max_possible_prefix = 0;
    for (std::size_t state = 0; state < automaton.GetStateNumber(); ++state) {
      if (automaton.IsAccepting(state)) {
        for (std::size_t prefix_length = 0; prefix_length <= pattern.size(); ++prefix_length) {
          if (is_possible_prefix[state][prefix_length]) {
            max_possible_prefix = std::max(max_possible_prefix, prefix_length);
          }
        }
      }
    }
    return max_possible_prefix;
  }

  std::size_t GetMaxMatchingPrefix(const regex::Regex &regex, const std::string &pattern) {
    auto automaton = automata::NondeterministicAutomaton::FromRegex(regex).RemoveEmptyTransitions();
    automaton.SplitTransitions();
    return GetMaxMatchingPrefix(automaton, pattern);
  }
}

#endif //AUTOMATA_STAT_H
