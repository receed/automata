#include "automaton.h"

void DeterministicAutomaton::AddTransition(std::size_t from_state, std::size_t to_state, TransitionString transition_symbol) {
  transitions_[from_state].emplace(transition_symbol, to_state);
}

std::optional<std::size_t> DeterministicAutomaton::GetNextState(std::size_t state, char symbol) {
  auto it = GetTransitions(state).find(symbol);
  if (it == GetTransitions(state).end())
    return std::nullopt;
  return it->second;
}

bool DeterministicAutomaton::AcceptsString(std::string_view string) {
  std::size_t current_state = *initial_state();
  for (char symbol: string) {
    auto next_state = GetNextState(current_state, symbol);
    if (!next_state)
      return false;
    current_state = *next_state;
  }
  return IsAccepting(current_state);
}

NondeterministicAutomaton &NondeterministicAutomaton::SplitTransitions() {
  auto state_number = GetStateNumber();
  for (std::size_t state = 0; state < state_number; ++state) {
    auto old_transitions = std::move(transitions_[state]);
    transitions_[state].clear();
    for (const auto&[transition_string, to_state]: old_transitions) {
      if (transition_string.size() <= 1)
        this->AddTransition(state, to_state, transition_string);
      else {
        auto last_added_state = state;
        for (std::size_t i = 0; i + 1 < transition_string.size(); ++i) {
          auto new_state = AddState();
          this->AddTransition(last_added_state, new_state, std::string(1, transition_string[i]));
          last_added_state = new_state;
        }
        this->AddTransition(last_added_state, to_state, std::string(1, transition_string.back()));
      }
    }
  }
  return *this;
}

NondeterministicAutomaton &NondeterministicAutomaton::RemoveEmptyTransitions() {
  auto state_number = GetStateNumber();
  for (std::size_t from_state = 0; from_state < state_number; ++from_state) {
    std::vector<bool> visited(state_number);
    visited[from_state] = true;
    std::vector<std::size_t> to_process{from_state};
    while (!to_process.empty()) {
      auto current_state = to_process.back();
      to_process.pop_back();
      for (const auto &[transition_string, to_state]: GetTransitions(current_state))
        if (transition_string.empty() && !visited[to_state]) {
          visited[to_state] = true;
          to_process.push_back(to_state);
        }
    }
    for (std::size_t to_state = 0; to_state < state_number; ++to_state)
      if (visited[to_state] && to_state != from_state) {
        if (IsAccepting(to_state))
          SetAccepting(from_state);
        this->AddTransition(from_state, to_state, "");
        for (const auto &[transition_string, next_state]: GetTransitions(to_state))
          this->AddTransition(from_state, next_state, transition_string);
      }
    RemoveDuplicateTransitions(from_state);
  }
  return *this;
}
