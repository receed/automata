#include "automaton.h"
#include "regex.h"
#include <vector>
#include <algorithm>

void DeterministicAutomaton::AddTransition(std::size_t from_state, std::size_t to_state,
                                           TransitionString transition_symbol) {
  AddTransition(from_state, to_state, transition_symbol);
}

bool DeterministicAutomaton::HasTransition(std::size_t state, char symbol) {
  return GetTransitions(state).count(symbol);
}

std::optional<std::size_t> DeterministicAutomaton::GetNextState(std::size_t state, char symbol) {
  auto it = GetTransitions(state).find(symbol);
  if (it == GetTransitions(state).end())
    return std::nullopt;
  return it->second;
}

bool DeterministicAutomaton::AcceptsString(std::string_view string) {
  std::size_t current_state = initial_state();
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

NondeterministicAutomaton DeterministicAutomaton::ToNondeterministic() {
  NondeterministicAutomaton result(initial_state(), is_accepting());
  ForEachTransition([&result](auto from_state, auto to_state, auto transition_symbol) {
    result.AddTransition(from_state, to_state, std::string(1, transition_symbol));
  });
  return result;
}

DeterministicAutomaton DeterministicAutomaton::Minimize() const {
  std::vector<std::size_t> class_indexes(GetStateNumber());
  std::size_t class_number = 0;
  auto reachable_states = GetReachableStates();
  for (std::size_t state = 0; state < GetStateNumber(); ++state)
    if (IsAccepting(state) != IsAccepting(0)) {
      class_number = 1;
      class_indexes[state] = 1;
    }
  while (true) {
    std::vector<std::size_t> new_class_indexes(GetStateNumber());
    std::map<std::vector<std::size_t>, std::size_t> index_of_class;
    for (std::size_t state = 0; state < GetStateNumber(); ++state) {
      assert(GetTransitions(state).size() == GetTransitions(0).size());
      std::vector<std::size_t> new_class{class_indexes[state]};
      for (auto [transition_symbol, to_state] : GetTransitions(state))
        new_class.push_back(class_indexes[to_state]);
      auto it = index_of_class.find(new_class);
      if (it != index_of_class.end())
        new_class_indexes[state] = it->second;
      else {
        new_class_indexes[state] = index_of_class.size();
        index_of_class[new_class] = new_class_indexes[state];
      }
    }
    class_number = index_of_class.size();
    if (new_class_indexes == class_indexes)
      break;
    class_indexes = new_class_indexes;
  }
  DeterministicAutomaton minimized_automaton{class_number, class_indexes[initial_state()]};
  for (std::size_t state = 0; state < GetStateNumber(); ++state)
    minimized_automaton.SetAccepting(class_indexes[state], IsAccepting(state));
  ForEachTransition([&minimized_automaton, &class_indexes](auto from_state, auto to_state, auto transition_symbol) {
    minimized_automaton.AddTransition(class_indexes[from_state], class_indexes[to_state], transition_symbol);
  });
  return minimized_automaton;
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
  initial_subset[initial_state()] = true;
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

regex::RegexPtr NondeterministicAutomaton::ToRegex() const {
  auto state_number = GetStateNumber();
  auto regex_transitions = std::vector(state_number, std::vector<regex::RegexPtr>(state_number, regex::create<regex::None>()));
  ForEachTransition([&regex_transitions](auto from_state, auto to_state, auto transition_string) {
    for (char symbol : transition_string)
      regex_transitions[from_state][to_state] += regex::create<regex::Literal>(symbol);
  });

  std::optional<std::size_t> accepting_state;
  for (std::size_t state = 0; state < GetStateNumber(); ++state) {
    if (state == initial_state())
      continue;
    if (IsAccepting(state)) {
      assert(!accepting_state);
      accepting_state = state;
      continue;
    }
    for (std::size_t from_state = 0; from_state < state_number; ++from_state)
      for (std::size_t to_state = 0; to_state < state_number; ++to_state) {
        if (from_state == state || to_state == state)
          continue;
        auto shortcut_regex =
            regex_transitions[from_state][state] *
            regex::Iterate(regex_transitions[state][state]) *
            regex_transitions[state][to_state];
        regex_transitions[from_state][to_state] += shortcut_regex;
      }
    for (std::size_t other_state = 0; other_state < state_number; ++other_state)
      regex_transitions[other_state][state] = regex_transitions[state][other_state] = regex::create<regex::None>();
  }
  if (!accepting_state)
    return regex::create<regex::None>();
  if (initial_state() == *accepting_state)
    return regex::Iterate(regex_transitions[initial_state()][initial_state()]);
  auto initial_to_accepting =
      regex::Iterate(regex_transitions[initial_state()][initial_state()]) * regex_transitions[initial_state()][*accepting_state];
  auto result = initial_to_accepting * regex::Iterate(regex_transitions[*accepting_state][*accepting_state] +
                                        regex_transitions[*accepting_state][initial_state()] *
                                        initial_to_accepting);
  return result;
}

NondeterministicAutomaton AutomatonVisitor::Process(const regex::Literal &regex) {
  return {2, 0, {1}, {{0, 1, std::string(1, regex.symbol)}}};
}

NondeterministicAutomaton AutomatonVisitor::Process(const regex::None &regex) {
  return {2, 0, {1}, {}};
}

NondeterministicAutomaton AutomatonVisitor::Process(const regex::Empty &regex) {
  return {2, 0, {1}, {{0, 1, ""}}};
}

NondeterministicAutomaton AutomatonVisitor::Process(const regex::Concatenation &regex, NondeterministicAutomaton first,
                                                    NondeterministicAutomaton second) {
  auto result = first;
  MergeAutomatons(result, second);
  result.AddTransition(first.GetStateNumber() - 1, first.GetStateNumber(), "");
  return result;
}

NondeterministicAutomaton AutomatonVisitor::Process(const regex::Alteration &regex, NondeterministicAutomaton first,
                                                    NondeterministicAutomaton second) {
  NondeterministicAutomaton result{1, 0};
  MergeAutomatons(result, first);
  MergeAutomatons(result, second);
  auto accepting = result.AddState();
  result.AddTransition(0, 1, "");
  result.AddTransition(0, first.GetStateNumber() + 1, "");
  result.AddTransition(first.GetStateNumber(), accepting, "");
  result.AddTransition(result.GetStateNumber() - 2, accepting, "");
}

NondeterministicAutomaton AutomatonVisitor::Process(const regex::KleeneStar &regex, NondeterministicAutomaton inner) {
  NondeterministicAutomaton result{1, 0};
  MergeAutomatons(result, inner);
  result.AddTransition(0, 1, "");
  result.AddTransition(result.GetStateNumber() - 1, 0);
  return result;
}
