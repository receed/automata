#include "regex.h"
#include <string>
#include <cassert>
#include <vector>
#include <variant>


namespace regex {
  void RegexNode::Print(std::size_t outer_priority, std::ostream &os) const {
    if (outer_priority > priority_)
      os << "(";
    Print(os);
    if (outer_priority > priority_)
      os << ")";
  }

  void None::Print(std::ostream &os) const {
    os << "0";
  }

  void Empty::Print(std::ostream &os) const {
    os << "1";
  }

  void Literal::Print(std::ostream &os) const {
    os << symbol;
  }

  void Concatenation::Print(std::ostream &os) const {
    GetChild(0).Print(priority_, os);
    GetChild(1).Print(priority_, os);
  }

  void Alteration::Print(std::ostream &os) const {
    GetChild(0).Print(priority_, os);
    os << '+';
    GetChild(1).Print(priority_, os);
  }

  void KleeneStar::Print(std::ostream &os) const {
    GetChild(0).Print(priority_, os);
    os << '*';
  }

  RegexPtr CreateLiteral(char symbol) {
    return create<Literal>(symbol);
  }

  RegexPtr Iterate(RegexPtr inner) {
    if (inner->IsNone() || inner->IsEmpty())
      return create<Empty>();
    return create<KleeneStar>(std::move(inner));
  }

  RegexPtr operator*(RegexPtr first, RegexPtr second) {
    if (first->IsNone() || second->IsEmpty())
      return std::move(first);
    if (second->IsNone() || first->IsEmpty())
      return std::move(second);
    return create<Concatenation>(std::move(first), std::move(second));
  }

  RegexPtr &operator*=(RegexPtr &first, RegexPtr second) {
    return first = std::move(first) * std::move(second);
  }

  RegexPtr operator+(RegexPtr first, RegexPtr second) {
    if (first->IsNone())
      return std::move(second);
    if (second->IsNone())
      return std::move(first);
    return create<Alteration>(std::move(first), std::move(second));
  }

  RegexPtr &operator+=(RegexPtr &first, RegexPtr second) {
    return first = std::move(first) + std::move(second);
  }

  RegexPtr parse(const std::string &input) {
    using Token = std::variant<RegexPtr, char>;
    std::vector<Token> stack;
    auto reduce_sum = [&stack]() {
      if (stack.size() < 3)
        return;
      auto first = std::get_if<regex::RegexPtr>(&stack[stack.size() - 3]);
      auto plus = std::get_if<char>(&stack[stack.size() - 2]);
      auto second = std::get_if<regex::RegexPtr>(&stack.back());
      if (first && plus && second && *plus == '+') {
        auto alteration = std::move(*first) + std::move(*second);
        stack.pop_back();
        stack.pop_back();
        stack.back() = alteration;
      }
    };
    for (char symbol: "(" + input + ")") {
      if (symbol == '*') {
        assert(!stack.empty() && std::holds_alternative<RegexPtr>(stack.back()));
        stack.back() = regex::Iterate(std::get<RegexPtr>(stack.back()));
        continue;
      }
      if (stack.size() >= 2 && std::holds_alternative<RegexPtr>(stack.back()) &&
          std::holds_alternative<RegexPtr>(stack[stack.size() - 2])) {
        std::get<RegexPtr>(stack[stack.size() - 2]) *= std::get<RegexPtr>(stack.back());
        stack.pop_back();
      }
      if (symbol == ')') {
        reduce_sum();
        assert(stack.size() >= 2);
        auto parenthesis = std::get_if<char>(&stack[stack.size() - 2]);
        assert(parenthesis && *parenthesis == '(');
        stack[stack.size() - 2] = std::move(stack.back());
        stack.pop_back();
        continue;
      }
      if (symbol == '+') {
        reduce_sum();
        stack.emplace_back(symbol);
        continue;
      }
      if (symbol == '(') {
        stack.emplace_back(symbol);
        continue;
      }
      stack.emplace_back(create<regex::Literal>(symbol));
    }
    assert(stack.size() == 1);
    return std::get<RegexPtr>(stack[0]);
  }

  template<>
  RegexPtr create<Empty>() {
    static auto regex = std::make_shared<Empty>();
    return regex;
  }

  template<>
  RegexPtr create<None>() {
    static auto regex = std::make_shared<None>();
    return regex;
  }
}
