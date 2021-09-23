#include "automaton.h"

void DeterministicAutomaton::AddTransition(std::size_t from_state, std::size_t to_state,
                                           TransitionString transition_symbol) {
  transitions_[from_state].emplace(transition_symbol, to_state);
}

bool DeterministicAutomaton::HasTransition(std::size_t state, char symbol) {
  return transitions_[state].count(symbol);
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

DeterministicAutomaton &DeterministicAutomaton::MakeComplete(const std::vector<char> &alphabet) {
  auto sink_state = AddState();
  for (std::size_t state = 0; state < GetStateNumber(); ++state)
    for (char symbol: alphabet) {
      if (!HasTransition(state, symbol))
        AddTransition(state, sink_state, symbol);
    }
  return *this;
}

DeterministicAutomaton &DeterministicAutomaton::Complement() {
  for (std::size_t state = 0; state < GetStateNumber(); ++state)
    SetAccepting(state, !IsAccepting(state));
  return *this;
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

DeterministicAutomaton NondeterministicAutomaton::Determinize() const {
  std::vector<bool> initial_subset(GetStateNumber());
  initial_subset[*initial_state()] = true;
  std::map<std::vector<bool>, std::size_t> subset_indices{{initial_subset, 0}};
  std::vector<std::vector<bool>> to_process{initial_subset};
  DeterministicAutomaton determinized_automaton{1, 0};

  while (!to_process.empty()) {
    auto current_subset = std::move(to_process.back());
    to_process.pop_back();
    auto current_subset_index = subset_indices.at(current_subset);
    std::unordered_map<char, std::vector<bool>> subset_transitions;

    for (std::size_t state = 0; state < GetStateNumber(); ++state) {
      if (!current_subset[state])
        continue;
      if (IsAccepting(state))
        determinized_automaton.SetAccepting(current_subset_index);
      for (auto[transition_string, to_state]: GetTransitions(state)) {
        assert(transition_string.size() == 1);
        auto symbol = transition_string[0];
        if (!subset_transitions.count(symbol))
          subset_transitions[symbol] = std::vector<bool>(GetStateNumber());
        subset_transitions[symbol][to_state] = true;
      }
    }

    for (const auto &[symbol, to_subset]: subset_transitions) {
      std::size_t to_subset_index;
      if (subset_indices.count(to_subset))
        to_subset_index = subset_indices.at(to_subset);
      else {
        to_subset_index = determinized_automaton.AddState();
        subset_indices[to_subset] = to_subset_index;
        to_process.push_back(to_subset);
      }
      determinized_automaton.AddTransition(current_subset_index, to_subset_index, symbol);
    }
  }
  return determinized_automaton;
}
