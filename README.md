# Automata

A set of functions for automata and regular expression processing.

## CLI

Command syntax: `[command] [arguments]`.

The program stores a set of objects, each of them is an automaton or a regular expression. Most of the commands create a new object, and when an object is created, it is assigned the smallest unused integer identificator.

The available commands are:

* `regex` -- reads a regular expression from stdin
* `automaton` -- reads an automaton from stdin. Input format:
```
[number of states] [initial state]
[acceping state]...
[from state] [to state] [transition string]
...
```

States are zero-indexed.

* `add_state [id]` -- adds a new state to automaton `[id]`
* `add_transition [id] [from] [to] [string]` -- adds a new transition to automaton `[id]`
* `print [id]` -- prints object `[id]`
* `minimize [id]` -- minimizes automaton `[id]` (the results of this and subsequent commands are saved to new objects)
* `to_complete [id] [alphabet]` -- makes a deterministic automaton complete (such that for each state symbol from string `[alphabet]` there is a transition from the state by the symbol)
* `determinize [id]` -- converts NDFA to DFA
* `complement [id] [alphabet]` -- given a complete DFA, builds a coomplete DFA accepting all other words with letters from `[alphabet]`
* `complement [id] [alphabet]` -- same for regular expressions
* `intersection [id1] [id2]` -- builds a DFA accepting the words accepted by both given DFAs
* `to_regex [id]` -- builds a regular expression by an automaton
* `to_nfa [id]` -- builds a NFA by a regular expression
* `to_mcdfa [id]` -- builds a minimal complete FFA by a regular expression
