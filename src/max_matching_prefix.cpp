#include <cassert>
#include "max_matching_prefix.h"

MaxMatchingPrefixFinder::MaxMatchingPrefixFinder(const automata::NondeterministicAutomaton &automaton,
                                                 std::string_view pattern)
    : automaton_(automaton), pattern_(pattern),
      is_possible_prefix_(automaton_.GetStateNumber(), std::vector<bool>(pattern_.size() + 1)) {
}

std::size_t MaxMatchingPrefixFinder::GetMaxMatchingPrefix(const regex::Regex &regex, const std::string &pattern) {
  auto automaton = automata::NondeterministicAutomaton::FromRegex(regex).RemoveEmptyTransitions();
  automaton.SplitTransitions();
  return MaxMatchingPrefixFinder(automaton, pattern).ComputeMaxMatchingPrefix();
}

std::size_t MaxMatchingPrefixFinder::ComputeMaxMatchingPrefix() {
  is_possible_prefix_[automaton_.initial_state()][0] = true;
  to_process_.emplace(automaton_.initial_state(), 0);
  while (!to_process_.empty()) {
    auto[state, prefix_length] = to_process_.front();
    to_process_.pop();
    ProcessState(state, prefix_length);
  }
  return max_matching_prefix_;
}

void MaxMatchingPrefixFinder::ProcessState(size_t state, size_t prefix_length) {
  if (automaton_.IsAccepting(state)) {
    max_matching_prefix_ = std::max(max_matching_prefix_, prefix_length);
  }
  if (prefix_length == pattern_.size()) {
    return;
  }
  for (const auto &transition: automaton_.GetTransitions(state)) {
    ProcessTransition(prefix_length, transition);
  }
}

void MaxMatchingPrefixFinder::ProcessTransition(size_t prefix_length, const automata::Transition <std::string> &transition) {
  if (GetSingleTransitionSymbol(transition) == pattern_[prefix_length] &&
      !is_possible_prefix_[transition.to_state][prefix_length + 1]) {
    is_possible_prefix_[transition.to_state][prefix_length + 1] = true;
    to_process_.emplace(transition.to_state, prefix_length + 1);
  }
}

char MaxMatchingPrefixFinder::GetSingleTransitionSymbol(const automata::Transition <std::string> &transition) {
  if (transition.symbol.size() != 1) {
    throw automata::BadAutomatonException("Symbol of length not equal to 1");
  }
  return transition.symbol[0];
}
