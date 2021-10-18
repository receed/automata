#include "cli.h"
#include "max_matching_prefix.h"

int main() {
  std::string input_regex;
  std::string pattern;
  while (std::cin >> input_regex >> pattern) {
    auto regex = regex::Regex::ParseReversePolish(input_regex);
    auto max_prefix_length = MaxMatchingPrefixFinder::GetMaxMatchingPrefix(regex, pattern);

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
