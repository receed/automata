#ifndef AUTOMATA_REGEX_H
#define AUTOMATA_REGEX_H

#include <string>

class Regex {
public:
  Regex() : repr_("0"), outer_priority_(0) {}

  Regex(std::string string) : outer_priority_(repr_.size() > 1 ? 1 : 2) {
    if (string.empty())
      repr_ = "1";
    else
      repr_ = std::move(string);
  }

  Regex(std::string repr, std::size_t outer_priority) : repr_(std::move(repr)), outer_priority_(outer_priority) {}

  std::string repr() const {
    return repr_;
  }

  bool IsEmptySet() const {
    return repr_ == "0";
  }

  bool IsEmptyString() const {
    return repr_ == "1";
  }

  Regex operator+(const Regex &other) const;

  Regex &operator+=(const Regex &other);

  Regex operator*(const Regex &other) const;

  Regex KleeneStar();

private:
  std::string repr_;
  std::size_t outer_priority_;

  std::string parenthesize(std::size_t operation_priority) const;
};

#endif //AUTOMATA_REGEX_H
