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
  struct Transition {
    T symbol;
    std::size_t to_state;

    auto operator<=>(const Transition<T> &) const = default;
  };

  template<typename T>
  class TransitionVector : public std::vector<Transition<T>> {
  public:
    void Add(Transition<T> transition) {
      std::vector<Transition<T>>::push_back(transition);
    }
  };

  class TransitionMap : public std::map<char, std::size_t> {
  public:
    using Map = std::map<char, std::size_t>;

    void Add(Transition<char> transition) {
      Map::insert({transition.symbol, transition.to_state});
    }

    std::optional<std::size_t> GetTransition(char symbol) const {
      auto it = Map::find(symbol);
      if (it == Map::end()) {
        return std::nullopt;
      }
      return it->second;
    }

    class Iterator {
    public:
      Iterator(Map::const_iterator map_iterator) : map_iterator_(map_iterator) {}

      Iterator &operator++() {
        ++map_iterator_;
        return *this;
      }

      Transition<char> operator*() const {
        return {map_iterator_->first, map_iterator_->second};
      }

      bool operator==(const Iterator &other) const {
        return map_iterator_ == other.map_iterator_;
      }

    private:
      Map::const_iterator map_iterator_;
    };

    Iterator begin() const {
      return {Map::cbegin()};
    }

    Iterator end() const {
      return {Map::cend()};
    }
  };

  template<typename TransitionContainer>
  class Automaton {
  public:
    virtual ~Automaton() = default;

    using TransitionString = decltype((*std::declval<TransitionContainer>().begin()).symbol);

    struct ExtendedTransition {
      std::size_t from_state, to_state;
      TransitionString transition_string;
    };

    Automaton(std::size_t initial_state, std::vector<bool> is_accepting) :
        initial_state_(initial_state), is_accepting_(is_accepting), transitions_(is_accepting.size()) {}

    Automaton(std::size_t initial_state, std::vector<bool> is_accepting, std::vector<TransitionContainer> transitions) :
        initial_state_(initial_state), is_accepting_(std::move(is_accepting)), transitions_(std::move(transitions)) {
      if (transitions_.size() != is_accepting_.size()) {
        throw BadAutomatonException("Sizes of accepting states and transitions differ");
      }
    }

    Automaton(std::size_t state_number = 0, std::size_t initial_state = 0,
              const std::vector<std::size_t> &accepting_states = {},
              const std::vector<ExtendedTransition> &transitions = {}) :
        transitions_(state_number),
        is_accepting_(state_number),
        initial_state_(initial_state) {
      for (auto state: accepting_states) {
        SetAccepting(state);
      }
      for (const auto &transition: transitions) {
        this->AddTransition(transition.from_state, transition.to_state, transition.transition_string);
      }
    }

    bool operator==(const Automaton<TransitionContainer> &other) const {
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

    void AddTransition(std::size_t from_state, std::size_t to_state, TransitionString transition_symbol) {
      transitions_[from_state].Add(Transition<TransitionString>{transition_symbol, to_state});
    }

    const TransitionContainer &GetTransitions(std::size_t from_state) const {
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

    void ReadAcceptingState(std::istream &is) {
      std::size_t state;
      is >> state;
      SetAccepting(state);
    }

    template<typename F>
    void ForEachTransition(F &&function) const {
      for (std::size_t state = 0; state < GetStateNumber(); ++state) {
        for (const auto &transition: GetTransitions(state)) {
          function(state, transition.to_state, transition.symbol);
        }
      }
    }

    void RemoveDuplicateTransitions(std::size_t from_state) {
      auto &outgoing_transitions = this->transitions_[from_state];
      std::ranges::sort(outgoing_transitions);
      auto to_erase = std::ranges::unique(outgoing_transitions);
      outgoing_transitions.erase(to_erase.begin(), to_erase.end());
    }

  protected:
    template<typename Visitor>
    void Traverse(Visitor &&visitor) const {
      std::vector<bool> was_reached(GetStateNumber());
      std::stack<std::size_t> to_process;
      was_reached[initial_state()] = true;
      to_process.push(initial_state());
      while (!to_process.empty()) {
        auto state = to_process.top();
        to_process.pop();
        visitor(state);
        for (const auto &transition: GetTransitions(state)) {
          if (!was_reached[transition.to_state]) {
            was_reached[transition.to_state] = true;
            to_process.push(transition.to_state);
          }
        }
      }
    }

    std::vector<std::size_t> GetReachableStates() const {
      std::vector<std::size_t> reachable_states;
      Traverse([&reachable_states](auto state) {
        reachable_states.push_back(state);
      });
      return reachable_states;
    }

  private:
    std::size_t initial_state_;
    std::vector<bool> is_accepting_;
  protected:
    std::vector<TransitionContainer> transitions_;
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
      for (const auto &transition: automaton.GetTransitions(state)) {
        os << "  to " << transition.to_state << " by " << transition.symbol << '\n';
      }
    }
    return os;
  }

  void SkipNewline(std::istream &is);

  template<typename T>
  T Parse(std::istream &is) {
    std::size_t state_number;
    std::size_t initial_state;
    is >> state_number >> initial_state;
    T automaton{state_number, initial_state};
    SkipNewline(is);
    while (is.peek() != '\n') {
      automaton.ReadAcceptingState(is);
    }
    SkipNewline(is);
    while (is.peek() != '\n') {
      std::size_t from_state, to_state;
      typename T::TransitionString transition_string;
      is >> from_state >> to_state >> transition_string;
      automaton.AddTransition(from_state, to_state, transition_string);
      SkipNewline(is);
    }
    SkipNewline(is);
    return automaton;
  }

  class NondeterministicAutomaton;

  class DeterministicAutomaton : public Automaton<TransitionMap> {
  public:
    using Automaton<TransitionMap>::Automaton;

    bool HasTransition(std::size_t state, char symbol) const;

    std::optional<std::size_t> GetNextState(std::size_t state, char symbol) const;

    bool AcceptsString(std::string_view string) const;

    DeterministicAutomaton &MakeComplete(const std::vector<char> &alphabet);

    DeterministicAutomaton ToComplete(const std::vector<char> &alphabet) const;

    DeterministicAutomaton &Complement();

    DeterministicAutomaton Minimize() const;

    DeterministicAutomaton Intersection(const DeterministicAutomaton &other) const;

    bool IsComplete() const;

    bool IsIsomorphic(const DeterministicAutomaton &other) const;

    bool IsEquivalent(const DeterministicAutomaton &other) const;
  };

  class NondeterministicAutomaton : public Automaton<TransitionVector<std::string>> {
  public:
    using Automaton<TransitionVector<std::string>>::Automaton;

    NondeterministicAutomaton(const DeterministicAutomaton &deterministic);

    NondeterministicAutomaton &SplitTransitions();

    NondeterministicAutomaton RemoveEmptyTransitions() const;

    DeterministicAutomaton DeterminizeSingleLetterTransitions() const;

    DeterministicAutomaton Determinize() const;

    static NondeterministicAutomaton FromRegex(const regex::Regex &input);

    regex::Regex ToRegex() const;

    NondeterministicAutomaton &MakeSingleAcceptingState();
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
