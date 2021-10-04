#ifndef AUTOMATA_AUTOMATON_H
#define AUTOMATA_AUTOMATON_H

#include "regex.h"
#include "util.h"
#include <cstddef>
#include <utility>
#include <vector>
#include <ranges>
#include <map>
#include <algorithm>
#include <iostream>
#include <stack>

namespace automata {
  class BadAutomatonException : public std::runtime_error {
    using std::runtime_error::runtime_error;
  };

  template<typename T>
  class Automaton {
  public:
    virtual ~Automaton() = default;

    using TransitionString = std::decay_t<decltype(std::declval<T>().begin()->first)>;

    struct Transition {
      std::size_t from_state, to_state;
      TransitionString transition_string;
    };

    Automaton(std::size_t initial_state, std::vector<bool> is_accepting) :
        initial_state_(initial_state), is_accepting_(is_accepting), transitions_(is_accepting.size()) {}

    Automaton(std::size_t initial_state, std::vector<bool> is_accepting, std::vector<T> transitions) :
        initial_state_(initial_state), is_accepting_(std::move(is_accepting)), transitions_(std::move(transitions)) {
      if (transitions_.size() != is_accepting_.size()) {
        throw BadAutomatonException("Sizes of accepting states and transitions differ");
      }
    }

    explicit Automaton(std::size_t state_number = 0, std::size_t initial_state = 0,
                       const std::vector<std::size_t> &accepting_states = {}) : transitions_(state_number),
                                                                                is_accepting_(state_number),
                                                                                initial_state_(initial_state) {
      for (auto state: accepting_states) {
        SetAccepting(state);
      }
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

    std::size_t GetSingleAcceptingState() const {
      if (std::ranges::count(is_accepting_, true) > 1) {
        throw BadAutomatonException("More than 1 accepting state");
      }
      return std::ranges::find(is_accepting_, true) - is_accepting_.begin();
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

    std::size_t initial_state() const {
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
      for (std::size_t state = 0; state < GetStateNumber(); ++state) {
        for (const auto &[transition, to_state]: GetTransitions(state)) {
          function(state, to_state, transition);
        }
      }
    }

  protected:
    std::vector<std::size_t> GetReachableStates() const {
      std::vector<bool> is_reachable(GetStateNumber());
      std::stack<std::size_t> to_process;
      is_reachable[initial_state()] = true;
      to_process.push(initial_state());
      while (!to_process.empty()) {
        auto state = to_process.top();
        to_process.pop();
        for (const auto &[transition, to_state]: GetTransitions(state)) {
          if (!is_reachable[to_state]) {
            is_reachable[to_state] = true;
            to_process.push(to_state);
          }
        }
      }
      std::vector<std::size_t> reachable_states;
      for (std::size_t state = 0; state < GetStateNumber(); ++state) {
        if (is_reachable[state]) {
          reachable_states.push_back(state);
        }
      }
      return reachable_states;
    }

  private:
    std::size_t initial_state_;
    std::vector<bool> is_accepting_;
  protected:
    std::vector<T> transitions_;
  };

  template<typename T>
  std::ostream &operator<<(std::ostream &os, const Automaton<T> &automaton) {
    auto state_number = automaton.GetStateNumber();
    os << state_number << " states\n";
    os << "Initial state: " << automaton.initial_state() << "\n";
    for (std::size_t state = 0; state < state_number; ++state) {
      os << "State " << state;
      if (automaton.IsAccepting(state)) {
        os << " (accepting)";
      }
      os << ":\n";
      for (const auto &[transition_string, to_state]: automaton.GetTransitions(state)) {
        os << "  to " << to_state << " by " << transition_string << '\n';
      }
    }
    return os;
  }

  template<typename T>
  T Parse(std::istream &is) {
    std::size_t state_number;
    std::size_t initial_state;
    is >> state_number >> initial_state;
    T automaton{state_number, initial_state};
    if (is.get() != '\n') {
      throw InvalidInputException("\\n expected");
    }
    while (is.peek() != '\n') {
      std::size_t state;
      is >> state;
      automaton.SetAccepting(state);
    }
    if (is.get() != '\n') {
      throw InvalidInputException("\\n expected");
    }
    while (is.peek() != '\n') {
      std::size_t from_state, to_state;
      typename T::TransitionString transition_string;
      is >> from_state >> to_state >> transition_string;
      automaton.AddTransition(from_state, to_state, transition_string);
      if (is.get() != '\n') {
        throw InvalidInputException("\\n expected");
      }
    }
    if (is.get() != '\n') {
      throw InvalidInputException("\\n expected");
    }
    return automaton;
  }

  template<typename TransitionString>
  class AbstractAutomaton : public Automaton<std::vector<std::pair<TransitionString, std::size_t>>> {
  public:
    using Automaton<std::vector<std::pair<TransitionString, std::size_t>>>::Automaton;
    using Transition = typename Automaton<std::vector<std::pair<TransitionString, std::size_t>>>::Transition;

    AbstractAutomaton(std::size_t state_number, std::size_t initial_state,
                      const std::vector<std::size_t> &accepting_states, const std::vector<Transition> &transitions)
        : AbstractAutomaton(state_number, initial_state, accepting_states) {
      for (const auto &transition: transitions) {
        AddTransition(transition.from_state, transition.to_state, transition.transition_string);
      }
    }

    void AddTransition(std::size_t from_state, std::size_t to_state, TransitionString transition_string) final {
      if (from_state >= this->GetStateNumber() || to_state >= this->GetStateNumber()) {
        throw std::out_of_range("Invalid state index");
      }
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
      this->SetAccepting(accepting_state);
      for (std::size_t state = 0; state < state_number; ++state) {
        if (this->IsAccepting(state)) {
          AddTransition(state, accepting_state, {});
          this->SetAccepting(state, false);
        }
      }
      return *this;
    }

  };

  class NondeterministicAutomaton;

  class DeterministicAutomaton : public Automaton<std::unordered_map<char, std::size_t>> {
  public:
    using Automaton<std::unordered_map<char, std::size_t>>::Automaton;

    DeterministicAutomaton(std::size_t state_number, std::size_t initial_state,
                           const std::vector<std::size_t> &accepting_states, const std::vector<Transition> &transitions)
        : Automaton<std::unordered_map<char, std::size_t>>(state_number, initial_state, accepting_states) {
      for (const auto &[from_state, to_state, transition_string]: transitions) {
        AddTransition(from_state, to_state, transition_string);
      }
    }

    void AddTransition(std::size_t from_state, std::size_t to_state, TransitionString transition_symbol) final;

    bool HasTransition(std::size_t state, char symbol);

    std::optional<std::size_t> GetNextState(std::size_t state, char symbol);

    bool AcceptsString(std::string_view string);

    DeterministicAutomaton &MakeComplete(const std::vector<char> &alphabet);

    DeterministicAutomaton &Complement();

    NondeterministicAutomaton ToNondeterministic();

    DeterministicAutomaton Minimize() const;

    DeterministicAutomaton Intersection(const DeterministicAutomaton &other) const;

    bool IsComplete() const;
  };

  class NondeterministicAutomaton : public AbstractAutomaton<std::string> {
  public:
    using AbstractAutomaton<std::string>::AbstractAutomaton;

    NondeterministicAutomaton &SplitTransitions();

    NondeterministicAutomaton RemoveEmptyTransitions() const;

    DeterministicAutomaton DeterminizeSingleLetterTransitions() const;

    DeterministicAutomaton Determinize() const;

    static NondeterministicAutomaton FromRegex(const regex::Regex &input);

    regex::Regex ToRegex() const;
  };

  class AutomatonVisitor : public regex::AbstractVisitor<NondeterministicAutomaton> {
  public:
    NondeterministicAutomaton Process(const regex::None &regex) override;

    NondeterministicAutomaton Process(const regex::Empty &regex) override;

    NondeterministicAutomaton Process(const regex::Concatenation &regex, NondeterministicAutomaton first,
                                      NondeterministicAutomaton second) override;

    NondeterministicAutomaton Process(const regex::Literal &regex) override;

    NondeterministicAutomaton
    Process(const regex::Alteration &regex, NondeterministicAutomaton first, NondeterministicAutomaton second) override;

    NondeterministicAutomaton Process(const regex::KleeneStar &regex, NondeterministicAutomaton inner) override;

  private:
    static void MergeAutomatons(NondeterministicAutomaton &first, const NondeterministicAutomaton &second);
  };

  DeterministicAutomaton RegexToMCDFA(const regex::Regex &expression, const std::vector<char> &alphabet);

  regex::Regex RegexComplement(const regex::Regex &expression, const std::vector<char> &alphabet);
}

#endif //AUTOMATA_AUTOMATON_H
