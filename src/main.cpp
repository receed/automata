#include <cassert>
#include "cli.h"
#include "regex_stat.h"
#include "stat.h"

int main() {
  std::string input_regex;
  std::string pattern;
  while (std::cin >> input_regex >> pattern) {
    auto regex = regex::Regex::ParseReversePolish(input_regex);
    auto max_prefix_length = regex_stat::GetMaxMatchingPrefix(regex, pattern);
    auto max_prefix_length_fast = stat::GetMaxMatchingPrefix(regex, pattern);
    assert(max_prefix_length == max_prefix_length_fast);

    if (max_prefix_length < 0) {
      std::cout << "INF";
    } else {
      std::cout << max_prefix_length;
    }
    std::cout << std::endl;
  }
//  cli::CLI cli;
//  cli.Start();
}
