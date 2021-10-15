#include "cli.h"
#include "regex_stat.h"

int main() {
  auto regex = regex::Regex::Parse("a.*c");
  std::cout << regex_stat::GetMaxMatchingPrefix(regex, "aacdy");
//  cli::CLI cli;
//  cli.Start();
}
