#ifndef AUTOMATA_REGEXP_H
#define AUTOMATA_REGEXP_H

#include <variant>
#include <memory>
#include <iostream>
#include <sstream>

namespace regex {
  class Regex;

  using RegexPtr = std::shared_ptr<Regex>;

  struct Regex {
    explicit Regex(std::size_t priority) : priority_(priority) {}

    virtual ~Regex() = default;

    std::string ToString() const {
      std::ostringstream os;
      Print(os);
      return os.str();
    }

    virtual void Print(std::ostream &os) const = 0;

    virtual bool IsNone() {
      return false;
    }

    virtual bool IsEmpty() {
      return false;
    }

    void Print(std::size_t outer_priority, std::ostream &os) const;

    std::size_t priority_;
  };

  struct None : public Regex {
    None() : Regex(2) {}

    bool IsNone() override {
      return true;
    }

    void Print(std::ostream &os) const override;
  };

  struct Empty : public Regex {
    Empty() : Regex(2) {}

    bool IsEmpty() override {
      return true;
    }

    void Print(std::ostream &os) const override;
  };

  struct Literal : public Regex {
    explicit Literal(char symbol) : Regex(2), symbol(symbol) {}

    void Print(std::ostream &os) const override;

    char symbol;
  };

  struct Concatenation : public Regex {
    Concatenation(RegexPtr first, RegexPtr second) : Regex(1), first(std::move(first)), second(std::move(second)) {}

    void Print(std::ostream &os) const override;

    RegexPtr first;
    RegexPtr second;
  };

  struct Alteration : public Regex {
    Alteration(RegexPtr first, RegexPtr second) : Regex(0), first(std::move(first)), second(std::move(second)) {}

    void Print(std::ostream &os) const override;

    RegexPtr first;
    RegexPtr second;
  };

  struct KleeneStar : public Regex {
    explicit KleeneStar(RegexPtr inner) : Regex(2), inner(std::move(inner)) {}

    void Print(std::ostream &os) const override;

    RegexPtr inner;
  };

  RegexPtr CreateLiteral(char symbol);

  RegexPtr Iterate(RegexPtr inner);

  RegexPtr operator+(RegexPtr first, RegexPtr second);

  RegexPtr &operator+=(RegexPtr &first, RegexPtr second);

  RegexPtr operator*(RegexPtr first, RegexPtr second);

  RegexPtr &operator*=(RegexPtr &first, RegexPtr second);

  RegexPtr parse(const std::string &input);

  template<typename T, typename... Args>
  RegexPtr create(Args &&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
  }

  template<>
  RegexPtr create<None>();

  template<>
  RegexPtr create<Empty>();
}

#endif //AUTOMATA_REGEXP_H
