#include "regex.h"
#include "automaton.h"

int main() {
  auto a = regex::Parse("ab");
  AutomatonVisitor visitor;
  a.Visit(visitor);
  auto b = visitor.GetResult();
  std::cout << b;
}
