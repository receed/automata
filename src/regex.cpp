#include "regex.h"
#include "util.h"
#include <string>
#include <vector>
#include <variant>

namespace regex {
  void RegexNode::Print(std::size_t outer_priority, std::ostream &os) const {
    if (outer_priority > priority_) {
      os << "(";
    }
    Print(os);
    if (outer_priority > priority_) {
      os << ")";
    }
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

  Regex Regex::Parse(const std::string &input) {
    using Token = std::variant<Regex, char>;
    std::vector<Token> stack;
    auto reduce_sum = [&stack]() {
      if (stack.size() < 3) {
        return;
      }
      auto first = std::get_if<regex::Regex>(&stack[stack.size() - 3]);
      auto plus = std::get_if<char>(&stack[stack.size() - 2]);
      auto second = std::get_if<regex::Regex>(&stack.back());
      if (first && plus && second && *plus == '+') {
        auto alteration = *first + *second;
        stack.pop_back();
        stack.pop_back();
        stack.back() = alteration;
      }
    };
    for (char symbol: "(" + input + ")") {
      if (symbol == '*') {
        if (stack.empty() || !std::holds_alternative<Regex>(stack.back())) {
          throw InvalidInputException("No symbol before \"*\"");
        }
        stack.back() = std::get<Regex>(stack.back()).Iterate();
        continue;
      }
      if (stack.size() >= 2 && std::holds_alternative<Regex>(stack.back()) &&
          std::holds_alternative<Regex>(stack[stack.size() - 2])) {
        std::get<Regex>(stack[stack.size() - 2]) *= std::get<Regex>(stack.back());
        stack.pop_back();
      }
      if (symbol == ')') {
        reduce_sum();
        if (stack.size() < 2) {
          throw InvalidInputException("Invalid parentheses pattern");
        }
        auto parenthesis = std::get_if<char>(&stack[stack.size() - 2]);
        if (!parenthesis || *parenthesis != '(') {
          throw InvalidInputException("Invalid parentheses pattern");
        }
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
      } else if (symbol == '0') {
        stack.emplace_back(Create<None>());
      } else if (symbol == '1') {
        stack.emplace_back(Create<Empty>());
      } else {
        stack.emplace_back(Create<regex::Literal>(symbol));
      }
    }
    if (stack.size() != 1) {
      throw InvalidInputException("Mismatched operators");
    }
    return std::get<Regex>(stack[0]);
  }

  std::istream &operator>>(std::istream &is, Regex &expression) {
    std::string line;
    std::getline(is, line);
    expression = Regex::Parse(line);
    return is;
  }

  std::ostream &operator<<(std::ostream &os, const Regex &expression) {
    expression.Print(os);
    return os;
  }

  template<>
  Regex Create<Empty>() {
    static auto regex = Regex(std::make_shared<Empty>());
    return regex;
  }

  template<>
  Regex Create<None>() {
    static auto regex = Regex(std::make_shared<None>());
    return regex;
  }

  Regex::Regex() : Regex(Create<Empty>()) {}

  Regex regex::Regex::Iterate() {
    if (root_node_->IsNone() || root_node_->IsEmpty()) {
      return Create<Empty>();
    }
    return Create<KleeneStar>(root_node_);
  }

  Regex regex::Regex::operator+(const Regex &other) {
    if (other.root_node_->IsNone()) {
      return *this;
    }
    if (root_node_->IsNone()) {
      return other;
    }
    return Create<Alteration>(root_node_, other.root_node_);
  }

  Regex &regex::Regex::operator+=(const Regex &other) {
    return *this = *this + other;
  }

  Regex regex::Regex::operator*(const Regex &other) {
    if (root_node_->IsNone() || other.root_node_->IsEmpty()) {
      return *this;
    }
    if (other.root_node_->IsNone() || root_node_->IsEmpty()) {
      return other;
    }
    return Create<Concatenation>(root_node_, other.root_node_);
  }

  Regex &regex::Regex::operator*=(const Regex &other) {
    return *this = *this * other;
  }
}
