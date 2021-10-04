#include "automaton.h"
#include "regex.h"
#include <vector>
#include <algorithm>
#include <set>

namespace automata {
  void DeterministicAutomaton::AddTransition(std::size_t from_state, std::size_t to_state,
                                             TransitionString transition_symbol) {
    transitions_[from_state].emplace(transition_symbol, to_state);
  }

  bool DeterministicAutomaton::HasTransition(std::size_t state, char symbol) {
    return GetTransitions(state).count(symbol);
  }

  std::optional<std::size_t> DeterministicAutomaton::GetNextState(std::size_t state, char symbol) {
    auto it = GetTransitions(state).find(symbol);
    if (it == GetTransitions(state).end()) {
      return std::nullopt;
    }
    return it->second;
  }

  bool DeterministicAutomaton::AcceptsString(std::string_view string) {
    std::size_t current_state = initial_state();
    for (char symbol: string) {
      auto next_state = GetNextState(current_state, symbol);
      if (!next_state) {
        return false;
      }
      current_state = *next_state;
    }
    return IsAccepting(current_state);
  }

  DeterministicAutomaton &DeterministicAutomaton::MakeComplete(const std::vector<char> &alphabet) {
    auto alphabet_set = std::set(alphabet.begin(), alphabet.end());
    ForEachTransition([&alphabet_set](auto from_state, auto to_state, auto transition_symbol) {
      alphabet_set.insert(transition_symbol);
    });
    auto sink_state = AddState();
    for (std::size_t state = 0; state < GetStateNumber(); ++state) {
      for (char symbol: alphabet_set) {
        if (!HasTransition(state, symbol)) {
          AddTransition(state, sink_state, symbol);
        }
      }
    }
    return *this;
  }

  DeterministicAutomaton &DeterministicAutomaton::Complement() {
    if (!IsComplete()) {
      throw BadAutomatonException("Automaton for complement must be complete");
    }
    for (std::size_t state = 0; state < GetStateNumber(); ++state) {
      SetAccepting(state, !IsAccepting(state));
    }
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
    std::size_t class_number;
    auto reachable_states = GetReachableStates();
    for (std::size_t state = 0; state < GetStateNumber(); ++state) {
      if (IsAccepting(state) != IsAccepting(0)) {
        class_indexes[state] = 1;
      }
    }
    while (true) {
      std::vector<std::size_t> new_class_indexes(GetStateNumber());
      std::map<std::vector<std::size_t>, std::size_t> index_of_class;
      for (std::size_t state = 0; state < GetStateNumber(); ++state) {
        if (GetTransitions(state).size() != GetTransitions(0).size()) {
          throw BadAutomatonException("The given DFA is not complete");
        }
        std::vector<std::size_t> new_class{class_indexes[state]};
        for (auto[transition_symbol, to_state]: GetTransitions(state)) {
          new_class.push_back(class_indexes[to_state]);
        }
        auto it = index_of_class.find(new_class);
        if (it != index_of_class.end()) {
          new_class_indexes[state] = it->second;
        } else {
          new_class_indexes[state] = index_of_class.size();
          index_of_class[new_class] = new_class_indexes[state];
        }
      }
      class_number = index_of_class.size();
      if (new_class_indexes == class_indexes) {
        break;
      }
      class_indexes = new_class_indexes;
    }
    DeterministicAutomaton minimized_automaton{class_number, class_indexes[initial_state()]};
    for (std::size_t state = 0; state < GetStateNumber(); ++state) {
      minimized_automaton.SetAccepting(class_indexes[state], IsAccepting(state));
    }
    ForEachTransition([&minimized_automaton, &class_indexes](auto from_state, auto to_state, auto transition_symbol) {
      minimized_automaton.AddTransition(class_indexes[from_state], class_indexes[to_state], transition_symbol);
    });
    return minimized_automaton;
  }

  DeterministicAutomaton DeterministicAutomaton::Intersection(const DeterministicAutomaton &other) const {
    auto get_index = [step = other.GetStateNumber()](std::size_t this_state, std::size_t other_state) {
      return this_state * step + other_state;
    };
    DeterministicAutomaton result{GetStateNumber() * other.GetStateNumber(),
                                  get_index(initial_state(), other.initial_state())};
    for (std::size_t this_from_state = 0; this_from_state < GetStateNumber(); ++this_from_state) {
      for (std::size_t other_from_state = 0; other_from_state < GetStateNumber(); ++other_from_state) {
        auto new_state = get_index(this_from_state, other_from_state);
        if (IsAccepting(this_from_state) && other.IsAccepting(other_from_state)) {
          result.SetAccepting(new_state);
        }
        const auto &other_transitions = other.GetTransitions(other_from_state);
        for (auto[symbol, this_to_state]: GetTransitions(this_from_state)) {
          auto it = other_transitions.find(symbol);
          if (it != other_transitions.end()) {
            result.AddTransition(new_state, get_index(this_to_state, it->second), symbol);
          }
        }
      }
    }
    return result;
  }

  bool DeterministicAutomaton::IsComplete() const {
    for (std::size_t state = 1; state < GetStateNumber(); ++state) {
      if (GetTransitions(state).size() != GetTransitions(0).size()) {
        return false;
      }
      for (auto[transition_symbol, to_state]: GetTransitions(state)) {
        if (!GetTransitions(0).count(transition_symbol)) {
          return false;
        }
      }
    }
    return true;
  }


  NondeterministicAutomaton &NondeterministicAutomaton::SplitTransitions() {
    auto state_number = GetStateNumber();
    for (std::size_t state = 0; state < state_number; ++state) {
      auto old_transitions = std::move(transitions_[state]);
      transitions_[state].clear();
      for (const auto&[transition_string, to_state]: old_transitions) {
        if (transition_string.size() <= 1) {
          this->AddTransition(state, to_state, transition_string);
        } else {
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

  NondeterministicAutomaton NondeterministicAutomaton::RemoveEmptyTransitions() const {
    auto state_number = GetStateNumber();
    NondeterministicAutomaton result{state_number, initial_state()};
    for (std::size_t from_state = 0; from_state < state_number; ++from_state) {
      std::vector<bool> visited(state_number);
      visited[from_state] = true;
      std::vector<std::size_t> to_process{from_state};
      while (!to_process.empty()) {
        auto current_state = to_process.back();
        to_process.pop_back();
        for (const auto &[transition_string, to_state]: GetTransitions(current_state)) {
          if (transition_string.empty() && !visited[to_state]) {
            visited[to_state] = true;
            to_process.push_back(to_state);
          }
        }
      }
      for (std::size_t to_state = 0; to_state < state_number; ++to_state) {
        if (visited[to_state]) {
          if (IsAccepting(to_state)) {
            result.SetAccepting(from_state);
          }
          for (const auto &[transition_string, next_state]: GetTransitions(to_state)) {
            if (!transition_string.empty()) {
              result.AddTransition(from_state, next_state, transition_string);
            }
          }
        }
      }
      result.RemoveDuplicateTransitions(from_state);
    }
    return result;
  }

  DeterministicAutomaton NondeterministicAutomaton::DeterminizeSingleLetterTransitions() const {
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
        if (!current_subset[state]) {
          continue;
        }
        if (IsAccepting(state)) {
          determinized_automaton.SetAccepting(current_subset_index);
        }
        for (auto[transition_string, to_state]: GetTransitions(state)) {
          if (transition_string.size() != 1) {
            throw BadAutomatonException("Transition is not single-letter");
          }
          auto symbol = transition_string[0];
          if (!subset_transitions.count(symbol)) {
            subset_transitions[symbol] = std::vector<bool>(GetStateNumber());
          }
          subset_transitions[symbol][to_state] = true;
        }
      }

      for (const auto &[symbol, to_subset]: subset_transitions) {
        std::size_t to_subset_index;
        if (subset_indices.count(to_subset)) {
          to_subset_index = subset_indices.at(to_subset);
        } else {
          to_subset_index = determinized_automaton.AddState();
          subset_indices[to_subset] = to_subset_index;
          to_process.push_back(to_subset);
        }
        determinized_automaton.AddTransition(current_subset_index, to_subset_index, symbol);
      }
    }
    return determinized_automaton;
  }

  DeterministicAutomaton NondeterministicAutomaton::Determinize() const {
    auto result = RemoveEmptyTransitions();
    result.SplitTransitions();
    return result.DeterminizeSingleLetterTransitions();
  }

  NondeterministicAutomaton NondeterministicAutomaton::FromRegex(const regex::Regex &input) {
    AutomatonVisitor visitor;
    input.Visit(visitor);
    auto automaton = visitor.GetResult();
    automaton.SetAccepting(automaton.GetStateNumber() - 1);
    return automaton;
  }

  regex::Regex NondeterministicAutomaton::ToRegex() const {
    auto state_number = GetStateNumber();
    auto regex_transitions = std::vector(state_number, std::vector<regex::Regex>(state_number,
                                                                                 regex::Create<regex::None>()));
    ForEachTransition([&regex_transitions](auto from_state, auto to_state, auto transition_string) {
      if (transition_string.size() >= 2) {
        throw BadAutomatonException("Length of transition string should be 0 or 1");
      }
      if (transition_string.empty()) {
        regex_transitions[from_state][to_state] += regex::Create<regex::Empty>();
      } else {
        regex_transitions[from_state][to_state] += regex::Create<regex::Literal>(transition_string[0]);
      }
    });

    std::optional<std::size_t> accepting_state;
    for (std::size_t state = 0; state < GetStateNumber(); ++state) {
      if (state == initial_state()) {
        continue;
      }
      if (IsAccepting(state)) {
        if (accepting_state) {
          throw BadAutomatonException("More than one accepting state");
        }
        accepting_state = state;
        continue;
      }
      for (std::size_t from_state = 0; from_state < state_number; ++from_state) {
        for (std::size_t to_state = 0; to_state < state_number; ++to_state) {
          if (from_state == state || to_state == state) {
            continue;
          }
          auto shortcut_regex =
              regex_transitions[from_state][state] *
              regex_transitions[state][state].Iterate() *
              regex_transitions[state][to_state];
          regex_transitions[from_state][to_state] += shortcut_regex;
        }
      }
      for (std::size_t other_state = 0; other_state < state_number; ++other_state) {
        regex_transitions[other_state][state] = regex_transitions[state][other_state] = regex::Create<regex::None>();
      }
    }
    if (!accepting_state) {
      return regex::Create<regex::None>();
    }
    if (initial_state() == *accepting_state) {
      return regex_transitions[initial_state()][initial_state()].Iterate();
    }
    auto initial_to_accepting =
        regex_transitions[initial_state()][initial_state()].Iterate() *
        regex_transitions[initial_state()][*accepting_state];
    return initial_to_accepting * (regex_transitions[*accepting_state][*accepting_state] +
                                   regex_transitions[*accepting_state][initial_state()] *
                                   initial_to_accepting).Iterate();
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

  NondeterministicAutomaton
  AutomatonVisitor::Process(const regex::Concatenation &regex, NondeterministicAutomaton first,
                            NondeterministicAutomaton second) {
    auto offset = first.GetStateNumber();
    auto first_accepting = first.GetSingleAcceptingState();
    first.SetAccepting(first_accepting, false);
    MergeAutomatons(first, second);
    first.AddTransition(first_accepting, offset, "");
    first.SetAccepting(offset + second.GetSingleAcceptingState());
    return first;
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
    result.SetAccepting(accepting);
    return result;
  }

  NondeterministicAutomaton AutomatonVisitor::Process(const regex::KleeneStar &regex, NondeterministicAutomaton inner) {
    NondeterministicAutomaton result{1, 0};
    MergeAutomatons(result, inner);
    result.AddTransition(0, 1, "");
    result.AddTransition(result.GetStateNumber() - 1, 0, "");
    result.SetAccepting(0);
    return result;
  }

  void AutomatonVisitor::MergeAutomatons(NondeterministicAutomaton &first, const NondeterministicAutomaton &second) {
    auto offset = first.GetStateNumber();
    for (std::size_t state = 0; state < second.GetStateNumber(); ++state) {
      first.AddState();
    }
    second.ForEachTransition([&first, offset](auto from_state, auto to_state, const auto &transition_string) {
      first.AddTransition(from_state + offset, to_state + offset, transition_string);
    });
  }

  DeterministicAutomaton RegexToMCDFA(const regex::Regex &expression, const std::vector<char> &alphabet) {
    return NondeterministicAutomaton::FromRegex(expression).Determinize().MakeComplete(
        alphabet).Minimize();
  }

  regex::Regex RegexComplement(const regex::Regex &expression, const std::vector<char> &alphabet) {
    auto automata = RegexToMCDFA(expression, alphabet).Complement().ToNondeterministic();
    automata.MakeSingleAcceptingState();
    return automata.ToRegex();
  }
}