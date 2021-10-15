#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.h"
#include "automaton.h"

using namespace automata;

TEST_SUITE("Automaton I/O") {
  TEST_CASE("Add accepting state") {
    DeterministicAutomaton automaton{3, 1, {0}};
    automaton.SetAccepting(1);
    CHECK_EQ(std::vector<bool>{1, 1, 0}, automaton.is_accepting());
  }

  TEST_CASE("Remove accepting state") {
    DeterministicAutomaton automaton{3, 1, {0, 1}};
    automaton.SetAccepting(1, false);
    CHECK_EQ(std::vector<bool>{1, 0, 0}, automaton.is_accepting());
  }

  TEST_CASE("Read accepting state") {
    std::istringstream input("1 2");
    NondeterministicAutomaton automaton{3, 0};
    automaton.ReadAcceptingState(input);
    CHECK_EQ(std::vector<bool>{0, 1, 0}, automaton.is_accepting());
  }

  TEST_CASE("Read automaton") {
    std::istringstream input("3 1\n0 2\n1 2 a\n1 0 b\n\n");
    CHECK_EQ(DeterministicAutomaton{3, 1, {0, 2}, {{1, 2, 'a'}, {1, 0, 'b'}}}, Parse<DeterministicAutomaton>(input));
  }

  TEST_CASE("Print automaton") {
    DeterministicAutomaton automaton{3, 1, {0, 2}, {{1, 2, 'a'}, {1, 0, 'b'}}};
    std::ostringstream output;
    output << automaton;
    std::cout << automaton;
    std::string expected = "3 states\nInitial state: 1\nState 0 (accepting):\nState 1:\n  to 2 by a\n  to 0 by b\nState 2 (accepting):\n";
    CHECK_EQ(expected, output.str());
  }
}

TEST_SUITE("Split transitions") {
  TEST_CASE("Short transitions") {
    CHECK_EQ(
        NondeterministicAutomaton{3, 0, {1}, {{0, 1, ""}, {1, 2, "a"}}}.SplitTransitions(),
        NondeterministicAutomaton{3, 0, {1}, {{0, 1, ""}, {1, 2, "a"}}}
    );
  }

  TEST_CASE("Long transitions") {
    CHECK_EQ(
        NondeterministicAutomaton{3, 0, {1}, {{0, 2, "abcd"}, {0, 1, "xy"}}}.SplitTransitions(),
        NondeterministicAutomaton{7, 0, {1},
                                  {{0, 3, "a"}, {3, 4, "b"}, {4, 5, "c"}, {5, 2, "d"}, {0, 6, "x"}, {6, 1, "y"}}}
    );
  }
}

TEST_CASE("Remove empty transitions") {
  CHECK_EQ(
      NondeterministicAutomaton{5, 0, {1}, {{1, 0, ""}, {2, 1, ""}, {3, 2, ""}, {1, 4, "ab"}}}.RemoveEmptyTransitions(),
      NondeterministicAutomaton{5, 0, {1, 2, 3}, {{1, 4, "ab"}, {2, 4, "ab"}, {3, 4, "ab"}}}
  );
}

TEST_SUITE("Determinize") {
  TEST_CASE("Redundant state") {
    CHECK_EQ(
        NondeterministicAutomaton{5, 0, {3, 4}, {{0, 1, "a"}, {0, 2, "a"}, {1, 3, "b"}, {2, 4, "c"}}}.Determinize(),
        DeterministicAutomaton{4, 0, {2, 3}, {{0, 1, 'a'}, {1, 3, 'b'}, {1, 2, 'c'}}}
    );
  }

  TEST_CASE("Loop") {
    CHECK_EQ(
        NondeterministicAutomaton{2, 0, {1}, {{0, 1, "a"}, {0, 0, "a"}}}.Determinize(),
        DeterministicAutomaton{2, 0, {1}, {{0, 1, 'a'}, {1, 1, 'a'}}}
    );
  }

  TEST_CASE("Unreachable state") {
    CHECK_EQ(
        NondeterministicAutomaton{2, 0, {0}, {{1, 0, "a"}}}.Determinize(),
        DeterministicAutomaton{1, 0, {0}, {}}
    );
  }

  TEST_CASE("Need additional state") {
    CHECK_EQ(
        NondeterministicAutomaton{3, 2, {0},
                                  {{2, 0, "a"}, {2, 1, "a"}, {2, 0, "b"}, {0, 1, "b"}, {1, 0, "b"}}}.Determinize(),
        DeterministicAutomaton{4, 0, {1, 2}, {{0, 2, 'a'}, {0, 1, 'b'}, {1, 3, 'b'}, {2, 2, 'b'}, {3, 1, 'b'}}}
    );
  }
}

TEST_CASE("Make complete") {
  CHECK_EQ(
      DeterministicAutomaton{3, 0, {2}, {{0, 1, 'a'}, {0, 2, 'b'}, {1, 2, 'a'}}}.MakeComplete({'a', 'b'}),
      DeterministicAutomaton{4, 0, {2},
                             {{0, 1, 'a'}, {0, 2, 'b'}, {1, 2, 'a'}, {1, 3, 'b'}, {2, 3, 'a'}, {2, 3, 'b'}, {3, 3, 'a'},
                              {3, 3, 'b'}}}
  );
}

TEST_CASE("Complement") {
  CHECK_EQ(
      DeterministicAutomaton{4, 0, {0, 2},
                             {{0, 1, 'a'}, {0, 2, 'b'}, {1, 2, 'a'}, {1, 3, 'b'}, {2, 3, 'a'}, {2, 3, 'b'}, {3, 3, 'a'},
                              {3, 3, 'b'}}}.Complement(),
      DeterministicAutomaton{4, 0, {1, 3},
                             {{0, 1, 'a'}, {0, 2, 'b'}, {1, 2, 'a'}, {1, 3, 'b'}, {2, 3, 'a'}, {2, 3, 'b'}, {3, 3, 'a'},
                              {3, 3, 'b'}}}
  );
}

TEST_SUITE("Build regular expression by automaton") {
  TEST_CASE("Single letter") {
    CHECK_EQ(
        NondeterministicAutomaton{2, 0, {1}, {{0, 1, "a"}}}.ToRegex().ToString(),
        "a"
    );
  }

  TEST_CASE("No cycles") {
    CHECK_EQ(
        NondeterministicAutomaton{3, 0, {2}, {{0, 1, "a"}, {0, 1, "b"}, {1, 2, "c"}}}.ToRegex().ToString(),
        "(a+b)c"
    );
  }

  TEST_CASE("Length 1 modulo 3") {
    CHECK_EQ(
        NondeterministicAutomaton{3, 0, {1}, {{0, 1, "a"}, {1, 2, "a"}, {2, 0, "a"}}}.ToRegex().ToString(),
        "a(aaa)*"
    );
  }

  TEST_CASE("Long regex") {
    CHECK_EQ(
        NondeterministicAutomaton{4, 1, {2}, {{1, 0, "a"}, {0, 3, "a"}, {0, 2, "b"}, {3, 2, "a"}, {3, 1, "b"},
                                              {2, 1, "a"}}}.ToRegex().ToString(),
        "(aab)*(ab+aaa)(a(aab)*(ab+aaa))*"
    );
  }

  TEST_CASE("Empty transition") {
    CHECK_EQ(
        NondeterministicAutomaton{3, 0, {2}, {{0, 1, "a"}, {1, 2, ""}}}.ToRegex().ToString(),
        "a"
    );
  }

}

TEST_SUITE("Minimization") {
  TEST_CASE("Identical vertices") {
    CHECK_EQ(
        DeterministicAutomaton{5, 0, {3}, {{0, 1, 'a'}, {0, 2, 'b'}, {1, 3, 'a'}, {1, 4, 'b'}, {2, 3, 'a'}, {2, 4, 'b'},
                                           {3, 4, 'a'}, {3, 4, 'b'}, {4, 4, 'a'}, {4, 4, 'b'}}}.Minimize(),
        DeterministicAutomaton{4, 0, {2}, {{0, 1, 'a'}, {0, 1, 'b'}, {1, 2, 'a'}, {1, 3, 'b'}, {2, 3, 'a'}, {2, 3, 'b'},
                                           {3, 3, 'a'}, {3, 3, 'b'}}}
    );
  }

  TEST_CASE("Identical groups") {
    CHECK_EQ(
        DeterministicAutomaton{6, 0, {1, 3},
                               {{0, 1, 'a'}, {0, 3, 'b'}, {1, 2, 'a'}, {1, 5, 'b'}, {2, 1, 'a'}, {2, 5, 'b'},
                                {3, 4, 'a'}, {3, 5, 'b'}, {4, 3, 'a'}, {4, 5, 'b'}, {5, 5, 'a'},
                                {5, 5, 'b'}}}.Minimize(),
        DeterministicAutomaton{4, 0, {1}, {{0, 1, 'a'}, {0, 1, 'b'}, {1, 2, 'a'}, {1, 3, 'b'}, {2, 1, 'a'}, {2, 3, 'b'},
                                           {3, 3, 'a'}, {3, 3, 'b'}}}
    );
  }

  TEST_CASE("Unreachable state") {
    CHECK_EQ(
        DeterministicAutomaton{3, 0, {2}, {{0, 0, 'a'}, {0, 2, 'b'}, {1, 0, 'a'}, {1, 2, 'b'}, {2, 2, 'a'},
                                           {2, 2, 'b'}}}.Minimize(),
        DeterministicAutomaton{2, 0, {1}, {{0, 0, 'a'}, {0, 1, 'b'}, {1, 1, 'a'}, {1, 1, 'b'}}}
    );
  }
}

TEST_SUITE("Automaton isomorphism check") {
  TEST_CASE("Different state count") {
    CHECK_FALSE(DeterministicAutomaton{1, 0, {0}, {}}.IsIsomorphic(DeterministicAutomaton{2, 0, {}, {}}));
  }

  TEST_CASE("Single accepting state") {
    CHECK(DeterministicAutomaton{1, 0, {0}, {}}.IsIsomorphic(DeterministicAutomaton{1, 0, {0}, {}}));
  }

  TEST_CASE("Single accepting and non-accepting states") {
    CHECK_FALSE(DeterministicAutomaton{1, 0, {0}, {}}.IsIsomorphic(DeterministicAutomaton{1, 0, {}, {}}));
  }

  TEST_CASE("Identical automata") {
    CHECK(
        DeterministicAutomaton{2, 0, {1}, {{0, 1, 'a'}}}.IsIsomorphic(
            DeterministicAutomaton{2, 0, {1}, {{0, 1, 'a'}}}));
  }

  TEST_CASE("Automata with and without transition") {
    CHECK_FALSE(DeterministicAutomaton{2, 0, {1}, {}}.IsIsomorphic(DeterministicAutomaton{2, 0, {1}, {{0, 1, 'a'}}}));
  }

  TEST_CASE("Renumbered states") {
    CHECK(DeterministicAutomaton{3, 0, {2}, {{0, 1, 'a'}, {0, 2, 'b'}}}.IsIsomorphic(
        DeterministicAutomaton{3, 1, {0}, {{1, 2, 'a'}, {1, 0, 'b'}}}));
  }

  TEST_CASE("Automaton accepts a subset of another one's language") {
    DeterministicAutomaton first{3, 0, {1}, {{0, 1, 'a'}, {1, 0, 'a'}}};
    DeterministicAutomaton second{3, 0, {1}, {{0, 1, 'a'}, {1, 2, 'a'}}};
    CHECK_FALSE(first.IsIsomorphic(second));
    CHECK_FALSE(second.IsIsomorphic(first));
  }
}

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
    CheckPrintAndParse(Create<Literal>('a') + Create<Literal>('b') + Create<Literal>('c'), "a+b+c");
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

TEST_SUITE("Print and Parse regex") {
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
                                  {{0, 1, ""}, {0, 4, ""}, {1, 2, ""}, {1, 6, ""}, {2, 3, "a"}, {3, 1, ""}, {4, 5, "b"}, {5, 6, ""}}}
    );
  }
}

TEST_CASE("Regex complement") {
  auto expression = regex::Regex::Parse("aa");
  CHECK_EQ(
      automata::RegexComplement(expression, {'a', 'b'}).ToString(),
      "1+a+(b+ab+aa(a+b))(a+b)*"
  );
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
  CHECK_EQ(
      first.Intersection(second),
      DeterministicAutomaton{4, 1, {2},
                             {{0, 2, 'a'}, {0, 1, 'b'}, {1, 3, 'a'}, {1, 0, 'b'}, {2, 0, 'a'}, {2, 3, 'b'}, {3, 1, 'a'},
                              {3, 2, 'b'}}});
}