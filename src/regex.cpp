#include "regex.h"

Regex Regex::operator+(const Regex &other) const {
  if (IsEmptySet())
    return other;
  if (other.IsEmptySet())
    return *this;
  return {repr_ + "+" + other.repr_, 0};
}

Regex &Regex::operator+=(const Regex &other) {
  return *this = *this + other;
}

Regex Regex::operator*(const Regex &other) const {
  if (IsEmptySet() || other.IsEmptySet())
    return {};
  if (IsEmptyString())
    return other;
  if (other.IsEmptyString())
    return *this;
  return {parenthesize(1) + other.parenthesize(1), 1};
}

Regex Regex::KleeneStar() {
  if (IsEmptySet() || IsEmptyString())
    return {"1"};
  return {parenthesize(2) + "*", 2};
}

std::string Regex::parenthesize(std::size_t operation_priority) const {
  if (outer_priority_ < operation_priority)
    return "(" + repr_ + ")";
  return repr_;
}
