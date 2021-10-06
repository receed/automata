#ifndef AUTOMATA_REGEXP_H
#define AUTOMATA_REGEXP_H

#include <memory>
#include <iostream>
#include <sstream>
#include <stack>
#include <span>
#include <ranges>
#include <algorithm>

namespace regex {
  class RegexNode;

  class Visitor;

  class Literal;

  class Empty;

  class None;

  class Alteration;

  class Concatenation;

  class KleeneStar;

  using RegexPtr = std::shared_ptr<RegexNode>;

  class Visitor {
  public:
    virtual ~Visitor() = default;

    virtual void Enter(const RegexNode &regex) {}

    virtual void Exit(const RegexNode &regex) {}

    virtual void Exit(const Literal &regex) {}

    virtual void Exit(const Empty &Empty) {}

    virtual void Exit(const None &None) {}

    virtual void Enter(const Alteration &regex) {}

    virtual void Exit(const Alteration &regex) {}

    virtual void Enter(const Concatenation &regex) {}

    virtual void Exit(const Concatenation &regex) {}

    virtual void Enter(const KleeneStar &regex) {}

    virtual void Exit(const KleeneStar &regex) {}
  };

  class RegexNode {
  public:
    explicit RegexNode(std::size_t priority) : priority_(priority) {}

    virtual ~RegexNode() = default;

    virtual void Print(std::ostream &os) const = 0;

    virtual bool IsNone() {
      return false;
    }

    virtual bool IsEmpty() {
      return false;
    }

    void Print(std::size_t outer_priority, std::ostream &os) const;

    virtual void Enter(Visitor &visitor) = 0;

    virtual void Exit(Visitor &visitor) = 0;

    virtual std::span<RegexPtr> children() = 0;

    std::size_t priority_;
  };

  template<typename T, std::size_t ChildrenCount>
  class BaseRegex : public RegexNode {
  public:
    template<typename ... Children>
    BaseRegex(std::size_t priority, Children... children) : RegexNode(priority), children_{std::move(children)...} {}

    void Enter(Visitor &visitor) override {
      visitor.Enter(static_cast<const T &>(*this));
    }

    void Exit(Visitor &visitor) override {
      visitor.Exit(static_cast<const T &>(*this));
    }

    const RegexNode &GetChild(std::size_t child_index) const {
      return *children_[child_index];
    }

    std::span<RegexPtr> children() override {
      return {children_, ChildrenCount};
    }

    RegexPtr children_[ChildrenCount];
  };

  class Regex {
  public:
    Regex(RegexPtr root_node) : root_node_(root_node) {}

    Regex();

    void Visit(Visitor &visitor) const {
      std::stack<std::pair<RegexPtr, bool>> stack;
      root_node_->Enter(visitor);
      stack.push({root_node_, false});
      while (!stack.empty()) {
        auto&[current_node, was_processed] = stack.top();
        if (was_processed) {
          current_node->Exit(visitor);
          stack.pop();
        } else {
          was_processed = true;
          current_node->Enter(visitor);
          for (const auto &child_node: std::ranges::reverse_view(current_node->children())) {
            stack.push({child_node, false});
          }
        }
      }
    }

    Regex Iterate();

    Regex operator+(const Regex &other) const;

    Regex &operator+=(const Regex &other);

    Regex operator*(const Regex &other) const;

    Regex &operator*=(const Regex &other);

    bool operator==(const Regex &other) const;

    void Print(std::ostream &os) const {
      root_node_->Print(os);
    }

    std::string ToString() const {
      std::ostringstream os;
      Print(os);
      return os.str();
    }

    static Regex Parse(const std::string &input);

  private:
    RegexPtr root_node_;
  };

  std::istream &operator>>(std::istream &is, Regex &expression);

  std::ostream &operator<<(std::ostream &os, const Regex &expression);

  struct None : public BaseRegex<None, 0> {
    None() : BaseRegex(2) {}

    bool IsNone() override {
      return true;
    }

    void Print(std::ostream &os) const override;
  };

  struct Empty : public BaseRegex<Empty, 0> {
    Empty() : BaseRegex(2) {}

    bool IsEmpty() override {
      return true;
    }

    void Print(std::ostream &os) const override;
  };

  struct Literal : public BaseRegex<Literal, 0> {
    explicit Literal(char symbol) : BaseRegex(2), symbol(symbol) {}

    void Print(std::ostream &os) const override;

    char symbol;
  };

  struct Concatenation : public BaseRegex<Concatenation, 2> {
    Concatenation(RegexPtr first, RegexPtr second) : BaseRegex(1, std::move(first), std::move(second)) {}

    void Print(std::ostream &os) const override;
  };

  struct Alteration : public BaseRegex<Alteration, 2> {
    Alteration(RegexPtr first, RegexPtr second) : BaseRegex(0, std::move(first), std::move(second)) {}

    void Print(std::ostream &os) const override;
  };

  struct KleeneStar : public BaseRegex<KleeneStar, 1> {
    explicit KleeneStar(RegexPtr inner) : BaseRegex(2, std::move(inner)) {}

    void Print(std::ostream &os) const override;
  };

  template<typename T, typename... Args>
  Regex Create(Args &&... args) {
    return {std::make_shared<T>(std::forward<Args>(args)...)};
  }

  template<>
  Regex Create<None>();

  template<>
  Regex Create<Empty>();

  template<typename T>
  class AbstractVisitor : public Visitor {
  public:
    T GetResult() {
      return stack_.back();
    }

    virtual T Process(const None &regex) = 0;

    virtual T Process(const Empty &regex) = 0;

    virtual T Process(const Literal &regex) = 0;

    virtual T Process(const Concatenation &regex, T first, T second) = 0;

    virtual T Process(const Alteration &regex, T first, T second) = 0;

    virtual T Process(const KleeneStar &regex, T inner) = 0;

    template<typename R, std::size_t ChildrenCount, std::size_t... Indices>
    void ProcessImpl(const BaseRegex<R, ChildrenCount> &regex, std::index_sequence<Indices...>) {
      auto result = Process(static_cast<const R &>(regex), stack_[stack_.size() - ChildrenCount + Indices]...);
      stack_.resize(stack_.size() - ChildrenCount);
      stack_.push_back(std::move(result));
    }

    template<typename R, std::size_t ChildrenCount>
    void ProcessImpl(const BaseRegex<R, ChildrenCount> &regex) {
      return ProcessImpl<R, ChildrenCount>(regex, std::make_index_sequence<ChildrenCount>());
    }

    void Exit(const Empty &regex) override {
      ProcessImpl(regex);
    }

    void Exit(const None &regex) override {
      ProcessImpl(regex);
    }

    void Exit(const Literal &regex) override {
      ProcessImpl(regex);
    }

    void Exit(const Alteration &regex)
    override {
      ProcessImpl(regex);
    }

    void Exit(const Concatenation &regex) override {
      ProcessImpl(regex);
    }

    void Exit(const KleeneStar &regex) override {
      ProcessImpl(regex);
    }

  private:
    std::vector<T> stack_;
  };
}

#endif //AUTOMATA_REGEXP_H
