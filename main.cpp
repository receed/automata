#include <iostream>
#include "automaton.h"

int main() {
  DeterministicAutomaton a;
  a.AddState();
  a.AddState();
  a.AddState();
  a.AddTransition(0, 1, 'a');
  a.AddTransition(1, 1, 'b');
  a.AddTransition(2, 1, 'a');
  a.SetInitialState(0);
  a.SetAccepting(1);
  std::cout << a;
  std::cout << a.AcceptsString("a") << '\n';
  std::cout << a.AcceptsString("abbb") << '\n';
  std::cout << a.AcceptsString("abbba") << '\n';
  NondeterministicAutomaton<std::string> b;
  b.AddState();
  b.AddState();
  b.AddState();
  b.AddTransition(0, 1, "aa");
  b.AddTransition(2, 0, "b");
  b.AddTransition(2, 1, "b");
  b.SetInitialState(0);
  b.SetAccepting(1);
  std::cout << b;
}


