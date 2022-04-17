#include "doctest.h"
#include "automaton.h"
#include "regex.h"

using namespace automata;
using namespace regex;

void CheckPrintAndParse(const Regex &regex, const std::string &representation) {
  CHECK_EQ(regex.ToString(), representation);
  CHECK_EQ(regex::Regex::Parse(representation).ToString(), representation);
}

TEST_SUITE("Print and Parse regex") {
  TEST_CASE("Empty set") {
    CheckPrintAndParse(Create<None>(), "0");
  }

  TEST_CASE("Empty string") {
    CheckPrintAndParse(Create<Empty>(), "1");
  }

  TEST_CASE("Simple string") {
    CheckPrintAndParse(Create<Literal>('a') * Create<Literal>('b') * Create<Literal>('c'), "abc");
  }

  TEST_CASE("Alteration") {
    CheckPrintAndParse(Create<Literal>('a')
                       + Create<Literal>('b') + Create<Literal>('c'), "a+b+c");
  }

  TEST_CASE("Kleene star") {
    CheckPrintAndParse(Create<Literal>('a').Iterate(), "a*");
  }

  TEST_CASE("Regex without parentheses") {
    CheckPrintAndParse(Create<Literal>('c') + Create<Literal>('a').Iterate() * Create<Literal>('b'), "c+a*b");
  }

  TEST_CASE("Regex with parentheses") {
    CheckPrintAndParse(((Create<Literal>('c') + Create<Literal>('a')) * Create<Literal>('b')).Iterate(), "((c+a)b)*");
  }
}

TEST_SUITE("Parse reverse Polish notation") {
  TEST_CASE("None") {
    CHECK_EQ(Create<None>(), Regex::ParseReversePolish("0"));
  }
  TEST_CASE("Empty string") {
    CHECK_EQ(Create<Empty>(), Regex::ParseReversePolish("1"));
  }
  TEST_CASE("Simple string") {
    CHECK_EQ(Create<Literal>('a') * Create<Literal>('b') * Create<Literal>('c'), Regex::ParseReversePolish("ab.c."));
  }
  TEST_CASE("Alteration") {
    CHECK_EQ(Create<Literal>('a') + Create<Literal>('b') + Create<Literal>('c'), Regex::ParseReversePolish("ab+c+"));
  }
  TEST_CASE("Kleene star") {
    CHECK_EQ(Create<Literal>('a').Iterate(), Regex::ParseReversePolish("a*"));
  }
  TEST_CASE("Long regex") {
    CHECK_EQ(Create<Literal>('c') + Create<Literal>('a').Iterate() * Create<Literal>('b'), Regex::ParseReversePolish("ca*b.+"));
  }
  TEST_CASE("No argument for *") {
    CHECK_THROWS_AS_MESSAGE(Regex::ParseReversePolish("*"), InvalidInputException, "No argument for *");
  }
  TEST_CASE("Not enough arguments for +") {
    CHECK_THROWS_AS_MESSAGE(Regex::ParseReversePolish("a+"), InvalidInputException, "Not enough arguments for +");
  }
  TEST_CASE("Not enough arguments for .") {
    CHECK_THROWS_AS_MESSAGE(Regex::ParseReversePolish("a."), InvalidInputException, "Not enough arguments for .");
  }
  TEST_CASE("Not all arguments are used in expression") {
    CHECK_THROWS_AS_MESSAGE(Regex::ParseReversePolish("ab"), InvalidInputException, "Not all arguments are used in expression");
  }
}

TEST_CASE("Input operator for regex") {
  std::istringstream is("c+a*b");
  Regex regex;
  is >> regex;
  CHECK_EQ(Create<Literal>('c') + Create<Literal>('a').Iterate() * Create<Literal>('b'), regex);
}

TEST_SUITE("Create automaton by regex") {
  TEST_CASE("Empty set") {
    CHECK_EQ(
        NondeterministicAutomaton::FromRegex(regex::Regex::Parse("0")),
        NondeterministicAutomaton{2, 0, {1}, {}}
    );
  }

  TEST_CASE("Empty string") {
    CHECK_EQ(
        NondeterministicAutomaton::FromRegex(regex::Regex::Parse("1")),
        NondeterministicAutomaton{2, 0, {1}, {{0, 1, ""}}}
    );
  }

  TEST_CASE("Single symbol") {
    CHECK_EQ(
        NondeterministicAutomaton::FromRegex(regex::Regex::Parse("a")),
        NondeterministicAutomaton{2, 0, {1}, {{0, 1, "a"}}}
    );
  }

  TEST_CASE("Concatenation") {
    CHECK_EQ(
        NondeterministicAutomaton::FromRegex(regex::Regex::Parse("ab")),
        NondeterministicAutomaton{4, 0, {3}, {{0, 1, "a"}, {1, 2, ""}, {2, 3, "b"}}}
    );
  }

  TEST_CASE("Alteration") {
    CHECK_EQ(
        NondeterministicAutomaton::FromRegex(regex::Regex::Parse("a+b")),
        NondeterministicAutomaton{6, 0, {5}, {{0, 1, ""}, {0, 3, ""}, {1, 2, "a"}, {3, 4, "b"}, {2, 5, ""}, {4, 5, ""}}}
    );
  }

  TEST_CASE("Compound regex") {
    CHECK_EQ(
        NondeterministicAutomaton::FromRegex(regex::Regex::Parse("a*+b")),
        NondeterministicAutomaton{7, 0, {6},
                                  {{0, 1, ""}, {0, 4, ""}, {1, 2, ""}, {1, 6, ""}, {2, 3, "a"}, {3, 1, ""}, {4, 5, "b"},
                                   {5, 6, ""}}}
    );
  }
}

TEST_CASE("Regex complement") {
  auto expression = regex::Regex::Parse("aa");
  CHECK_EQ(automata::RegexComplement(expression, {'a', 'b'}).ToString(), "1+a+(b+ab+aa(a+b))(a+b)*");
}

TEST_SUITE("Regex equivalence") {
  TEST_CASE("Same symbol") {
    CHECK(regex::Regex::Parse("a") == regex::Regex::Parse("a"));
  }

  TEST_CASE("Different symbols") {
    CHECK_FALSE(regex::Regex::Parse("a") == regex::Regex::Parse("b"));
  }

  TEST_CASE("Product Kleene star and symbol") {
    CHECK(regex::Regex::Parse("aa*") == regex::Regex::Parse("a*a"));
  }

  TEST_CASE("Symbol added next to Kleene star matters") {
    CHECK_FALSE(regex::Regex::Parse("a*") == regex::Regex::Parse("aa*"));
  }

  TEST_CASE("Alteration is commutative") {
    CHECK(regex::Regex::Parse("a+b") == regex::Regex::Parse("b+a"));
  }

  TEST_CASE("Concatenation is not commutative") {
    CHECK_FALSE(regex::Regex::Parse("ab") == regex::Regex::Parse("ba"));
  }

  TEST_CASE("Different ways to write aba...ba") {
    CHECK(regex::Regex::Parse("(ab)*a") == regex::Regex::Parse("a(ba)*"));
  }
}

TEST_CASE("Automaton intersection") {
  DeterministicAutomaton first{2, 0, {1}, {{0, 1, 'a'}, {1, 0, 'a'}, {0, 0, 'b'}, {1, 1, 'b'}}};
  DeterministicAutomaton second{2, 1, {0}, {{0, 1, 'b'}, {1, 0, 'b'}, {0, 0, 'a'}, {1, 1, 'a'}}};
  CHECK_EQ(first.Intersection(second), DeterministicAutomaton{4, 1, {2},
                                                              {{0, 2, 'a'}, {0, 1, 'b'}, {1, 3, 'a'}, {1, 0, 'b'},
                                                               {2, 0, 'a'}, {2, 3, 'b'}, {3, 1, 'a'}, {3, 2, 'b'}}});
}

