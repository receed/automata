#include <iostream>
#include "automaton.h"

int main() {
  Automaton<std::vector<std::pair<std::string, std::size_t>>> aa{1, 0, {}};
  AbstractAutomaton<std::string> ab{1, 0, {}, {}};

  DeterministicAutomaton a;
  a.AddState();
  a.AddState();
  a.AddState();
  a.AddTransition(0, 1, 'a');
  a.AddTransition(1, 1, 'b');
  a.AddTransition(2, 1, 'a');
  a.SetInitialState(0);a.SetAccepting(1);
  std::cout << a;
  std::cout << a.AcceptsString("a") << '\n';
  std::cout << a.AcceptsString("abbb") << '\n';
  std::cout << a.AcceptsString("abbba") << '\n';

  NondeterministicAutomaton b;
  b.AddState();
  b.AddState();
  b.AddState();
  b.AddTransition(0, 1, "abca");
  b.AddTransition(2, 0, "");
  b.AddTransition(2, 1, "b");
  b.AddTransition(2, 0, "xyz");
  b.SetInitialState(0);
  b.SetAccepting(1);
  std::cout << b;
  b.SplitTransitions();
  std::cout << b;
}


