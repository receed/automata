#ifndef AUTOMATA_REGEXP_H
#define AUTOMATA_REGEXP_H

#include <variant>
#include <memory>

namespace regex {
  struct Regex {

  };

  using RegexPtr = std::unique_ptr<Regex>;

  struct None : public Regex {
  };

  struct Empty : public Regex {
  };

  struct Literal : public Regex {
    explicit Literal(char symbol) : symbol(symbol) {}

    char symbol;
  };

  struct Concatenation : public Regex {
    Concatenation(RegexPtr &&first, RegexPtr &&second) : first(std::move(first)), second(std::move(second)) {}

    RegexPtr first;
    RegexPtr second;
  };

  struct Alteration : public Regex {
    RegexPtr first;
    RegexPtr second;
  };

  struct KleeneStar : public Regex {
    KleeneStar(RegexPtr &&inner) : inner(std::move(inner)) {}

    RegexPtr inner;
  };
}

#endif //AUTOMATA_REGEXP_H
