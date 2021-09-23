#ifndef AUTOMATA_AUTOMATON_H
#define AUTOMATA_AUTOMATON_H

#include <cstddef>
#include <utility>
#include <vector>
#include <ranges>
#include <map>
#include <cassert>
#include <algorithm>
#include <iostream>

template<typename T>
class Automaton {
public:
  using TransitionString = decltype(std::declval<T>().begin()->first);

  struct Transition {
    std::size_t from_state, to_state;
    TransitionString transition_string;
  };

  Automaton(std::optional<std::size_t> initial_state, std::vector<bool> is_accepting) :
      initial_state_(initial_state), is_accepting_(std::move(is_accepting)), transitions_(is_accepting_.size()) {}

  Automaton(std::optional<std::size_t> initial_state, std::vector<bool> is_accepting, std::vector<T> transitions) :
      initial_state_(initial_state), is_accepting_(std::move(is_accepting)), transitions_(std::move(transitions)) {
    assert(transitions_.size() == is_accepting_.size());
  }

  explicit Automaton(std::size_t state_number = 0, std::optional<std::size_t> initial_state = std::nullopt,
                     const std::vector<std::size_t> &accepting_states = {}) : transitions_(state_number),
                                                                              is_accepting_(state_number),
                                                                              initial_state_(initial_state) {
    for (auto state: accepting_states)
      SetAccepting(state);
  }

  bool operator==(const Automaton<T> &other) const {
    return transitions_ == other.transitions_ && initial_state_ == other.initial_state_ &&
           is_accepting_ == other.is_accepting_;
  }

  std::size_t GetStateNumber() const {
    return is_accepting_.size();
  }

  const std::vector<bool> &is_accepting() const {
    return is_accepting_;
  }

  bool IsAccepting(std::size_t state) const {
    return is_accepting_[state];
  }

  std::size_t AddState(bool final) {
    is_accepting_.push_back(final);
    transitions_.emplace_back();
    return GetStateNumber() - 1;
  }

  std::size_t AddState() {
    return AddState(false);
  }

  virtual void AddTransition(std::size_t from_state, std::size_t to_state, TransitionString transition_symbol) = 0;

  const T &GetTransitions(std::size_t from_state) const {
    return transitions_[from_state];
  }

  std::optional<std::size_t> initial_state() const {
    return initial_state_;
  }

  void SetInitialState(std::size_t initial_state) {
    initial_state_ = initial_state;
  }

  void SetAccepting(std::size_t state, bool accepting = true) {
    is_accepting_[state] = accepting;
  }

  template<typename F>
  void ForEachTransition(F &&function) const {
    for (std::size_t state = 0; state < GetStateNumber(); ++state)
      for (const auto &[transition, to_state]: GetTransitions(state))
        function(state, to_state, transition);
  }

protected:
  std::vector<T> transitions_;

private:
  std::vector<bool> is_accepting_;
  std::optional<std::size_t> initial_state_;
};

template<typename T>
std::ostream &operator<<(std::ostream &os, const Automaton<T> &automaton) {
  auto state_number = automaton.GetStateNumber();
  os << state_number << " states\n";
  auto initial_state = automaton.initial_state();
  os << "Initial state: ";
  if (initial_state)
    os << *initial_state << "\n";
  else
    os << "undefined \n";
  for (std::size_t state = 0; state < state_number; ++state) {
    os << "State " << state;
    if (automaton.IsAccepting(state))
      os << " (accepting)";
    os << ":\n";
    for (const auto &[transition_string, to_state]: automaton.GetTransitions(state))
      os << "  to " << to_state << " by " << transition_string << '\n';
  }
  return os;
}

template<typename TransitionString>
class AbstractAutomaton : public Automaton<std::vector<std::pair<TransitionString, std::size_t>>> {
public:
  using Automaton<std::vector<std::pair<TransitionString, std::size_t>>>::Automaton;
  using typename Automaton<std::vector<std::pair<TransitionString, std::size_t>>>::Transition;

  AbstractAutomaton(std::size_t state_number, std::optional<std::size_t> initial_state,
                    const std::vector<std::size_t> &accepting_states, const std::vector<Transition> &transitions)
      : AbstractAutomaton(state_number, initial_state, accepting_states) {
    for (const auto &transition: transitions)
      AddTransition(transition.from_state, transition.to_state, transition.transition_string);
  }

  void AddTransition(std::size_t from_state, std::size_t to_state, TransitionString transition_string) final {
    this->transitions_[from_state].emplace_back(std::move(transition_string), to_state);
  }

  void RemoveDuplicateTransitions(std::size_t from_state) {
    auto &outgoing_transitions = this->transitions_[from_state];
    std::ranges::sort(outgoing_transitions);
    auto to_erase = std::ranges::unique(outgoing_transitions);
    outgoing_transitions.erase(to_erase.begin(), to_erase.end());
  }

  AbstractAutomaton &MakeSingleAcceptingState() {
    auto state_number = this->GetStateNumber();
    auto accepting_state = this->AddState();
    this->SetAccepting(accepting_state, true);
    for (std::size_t state = 0; state < state_number; ++state)
      if (this->IsAccepting(state)) {
        AddTransition(state, accepting_state, {});
        this->SetAccepting(state, false);
      }
    return *this;
  }
};

class NondeterministicAutomaton;

class DeterministicAutomaton : public Automaton<std::unordered_map<char, std::size_t>> {
public:
  using Automaton<std::unordered_map<char, std::size_t>>::Automaton;

  DeterministicAutomaton(std::size_t state_number, std::optional<std::size_t> initial_state,
                         const std::vector<std::size_t> &accepting_states, const std::vector<Transition> &transitions)
      : DeterministicAutomaton(state_number, initial_state, accepting_states) {
    for (const auto &[from_state, to_state, transition_string]: transitions)
      AddTransition(from_state, to_state, transition_string);
  }

  void AddTransition(std::size_t from_state, std::size_t to_state, TransitionString transition_symbol) final;

  bool HasTransition(std::size_t state, char symbol);

  std::optional<std::size_t> GetNextState(std::size_t state, char symbol);

  bool AcceptsString(std::string_view string);

  DeterministicAutomaton &MakeComplete(const std::vector<char> &alphabet);

  DeterministicAutomaton &Complement();

  NondeterministicAutomaton ToNondeterministic();
};

class NondeterministicAutomaton : public AbstractAutomaton<std::string> {
public:
  using AbstractAutomaton<std::string>::AbstractAutomaton;

  NondeterministicAutomaton &SplitTransitions();

  NondeterministicAutomaton &RemoveEmptyTransitions();

  DeterministicAutomaton Determinize() const;

  std::string ToRegex() const;
};

#endif //AUTOMATA_AUTOMATON_H
