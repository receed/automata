#ifndef AUTOMATA_REGEX_STAT_H
#define AUTOMATA_REGEX_STAT_H

#include "regex.h"

namespace regex_stat {
  using PossibleSubstrings = std::vector<std::vector<bool>>;

  class StringVisitor : public regex::AbstractVisitor<PossibleSubstrings> {
  public:
    explicit StringVisitor(const std::string& pattern) : pattern_(pattern) {}

    PossibleSubstrings Process(const regex::None &regex) override {
      return GetValue();
    }

    PossibleSubstrings Process(const regex::Empty &regex) override {
      auto possible_substrings = GetValue();
      for (std::size_t position = 0; position <= pattern_.size(); ++position) {
        possible_substrings[position][position] = true;
      }
      return possible_substrings;
    }

    PossibleSubstrings Process(const regex::Literal &regex) override {
      auto possible_substrings = GetValue();
      for (std::size_t position = 0; position < pattern_.size(); ++position) {
        if (pattern_[position] == regex.symbol) {
          possible_substrings[position][position + 1] = true;
        }
      }
      return possible_substrings;
    }

    PossibleSubstrings Process(const regex::Concatenation &regex, PossibleSubstrings first, PossibleSubstrings second) override {
      auto possible_substrings = GetValue();
      for (std::size_t right = 0; right <= pattern_.size(); ++right) {
        for (std::size_t pivot = 0; pivot <= right; ++pivot) {
          for (std::size_t left = 0; left <= pivot; ++left) {
            if (first[left][pivot] && second[pivot][right]) {
              possible_substrings[left][right] = true;
            }
          }
        }
      }
      return possible_substrings;
    }

    PossibleSubstrings Process(const regex::Alteration &regex, PossibleSubstrings first, PossibleSubstrings second) override {
      for (std::size_t right = 0; right <= pattern_.size(); ++right) {
        for (std::size_t left = 0; left <= right; ++left) {
          if (second[left][right]) {
            first[left][right] = true;
          }
        }
      }
      return first;
    }

    PossibleSubstrings Process(const regex::KleeneStar &regex, PossibleSubstrings inner) override {
      MakeTransitiveClosure(inner);
      for (std::size_t position = 0; position <= pattern_.size(); ++position) {
        inner[position][position] = true;
      }
      return inner;
    }

  private:
    PossibleSubstrings GetValue() {
      return std::vector(pattern_.size() + 1, std::vector(pattern_.size() + 1, false));
    }

    void MakeTransitiveClosure(PossibleSubstrings &possible_substrings) {
      for (std::size_t left = pattern_.size() + 1; left-- > 0; ) {
        for (std::size_t right = left; right <= pattern_.size(); ++right) {
          for (std::size_t pivot = left; pivot <= right; ++pivot) {
            if (possible_substrings[left][pivot] && possible_substrings[pivot][right]) {
              possible_substrings[left][right] = true;
            }
          }
        }
      }
    }

    const std::string &pattern_;
  };

  std::size_t GetMaxMatchingPrefix(const regex::Regex &regex, const std::string &pattern) {
    StringVisitor visitor {pattern};
    regex.Visit(visitor);
    auto possible_substrings = visitor.GetResult();
    auto max_prefix = std::find(possible_substrings[0].rbegin(), possible_substrings[0].rend(), true);
    if (max_prefix == possible_substrings[0].rend()) {
      return 0;
    }
    return possible_substrings[0].rend() - max_prefix - 1;
  }
}

#endif //AUTOMATA_REGEX_STAT_H
