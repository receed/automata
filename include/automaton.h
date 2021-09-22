#ifndef AUTOMATA_AUTOMATON_H
#define AUTOMATA_AUTOMATON_H

#include <cstddef>
#include <vector>
#include <ranges>
#include <map>

template<typename T>
class Automaton {
public:
  using Transition = decltype(std::declval<T>().begin()->first);

  std::size_t GetStateNumber() const {
    return is_accepting_.size();
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

  virtual void AddTransition(std::size_t from_state, std::size_t to_state, Transition transition_symbol) = 0;

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

  std::vector<T> transitions_;

private:
  std::optional<std::size_t> initial_state_;
  std::vector<bool> is_accepting_;
};

template<typename T>
std::ostream &operator<<(std::ostream &os, const Automaton<T> &automaton) {
  auto state_count = automaton.GetStateNumber();
  os << state_count << " states\n";
  auto initial_state = automaton.initial_state();
  os << "Initial state: ";
  if (initial_state)
    os << *initial_state << "\n";
  else
    os << "undefined \n";
  for (std::size_t state = 0; state < state_count; ++state) {
    os << "State 0";
    if (automaton.IsAccepting(state))
      os << " (accepting)";
    os << ":\n";
    for (const auto &[transition, to_state]: automaton.GetTransitions(state))
      os << "  to " << to_state << " by " << transition << '\n';
  }
  return os;
}

template<typename Transition>
class NondeterministicAutomaton : public Automaton<std::vector<std::pair<Transition, std::size_t>>> {
public:
  void AddTransition(std::size_t from_state, std::size_t to_state, Transition transition_symbol) override {
    this->transitions_[from_state].emplace_back(std::move(transition_symbol), to_state);
  }
};

class DeterministicAutomaton : public Automaton<std::map<char, std::size_t>> {
public:
  void AddTransition(std::size_t from_state, std::size_t to_state, Transition transition_symbol) override {
    transitions_[from_state].emplace(transition_symbol, to_state);
  }

  std::optional<std::size_t> GetNextState(std::size_t state, char symbol) {
    auto it = GetTransitions(state).find(symbol);
    if (it == GetTransitions(state).end())
      return std::nullopt;
    return it->second;
  }

  bool AcceptsString(std::string_view string) {
    std::size_t current_state = *initial_state();
    for (char symbol : string) {
      auto next_state = GetNextState(current_state, symbol);
      if (!next_state)
        return false;
      current_state = *next_state;
    }
    return IsAccepting(current_state);
  }
};

#endif //AUTOMATA_AUTOMATON_H
