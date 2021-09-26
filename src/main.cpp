#include <iostream>
#include "automaton.h"
#include "regex.h"
#include "regex_class.h"

int main() {
  auto ra = std::make_unique<regex::Literal>('a');
//  auto rb = std::make_unique<regex::Literal>('b');
  auto k = regex::KleeneStar(std::move(ra));
  auto r = std::make_unique<regex::KleeneStar>(std::move(ra));
}


