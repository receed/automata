#ifndef AUTOMATA_CLI_H
#define AUTOMATA_CLI_H

#include <variant>
#include <iostream>
#include "automaton.h"
#include "regex.h"

namespace cli {
  using Object = std::variant<automata::NondeterministicAutomaton, automata::DeterministicAutomaton, regex::Regex>;
  namespace command {
    class AbstractCommandHandle;
  }

  automata::DeterministicAutomaton ToDfa(const Object& object);
  automata::NondeterministicAutomaton ToNfa(const Object& object);

  class CLI {
  public:
    CLI();

    std::size_t AddObject(Object object);

    template<typename T>
    T &GetObject(std::size_t id) {
      if constexpr(std::is_same_v<T, Object>) {
        return objects_.at(id);
      } else {
        auto object = std::get_if<T>(&objects_.at(id));
        if (!object) {
          throw InvalidInputException("Wrong type of argument");
        }
        return *object;
      }
    }

    void ExecuteCommand(const std::string &command_string);

    void Start();

    template<typename T>
    void AddCommandHandle(std::string name);

  private:
    std::vector<Object> objects_;
    std::vector<std::unique_ptr<command::AbstractCommandHandle>> command_handles;
  };


  namespace command {
    class Command {
    public:
      Command(CLI &cli, std::istream &args) : cli_(cli), args_(args) {}

      CLI &cli_;
      std::istream &args_;

      virtual ~Command() = default;

      virtual void Execute() = 0;

      template<typename T>
      T &GetObject() {
        std::size_t id;
        args_ >> id;
        return cli_.GetObject<T>(id);
      }
    };

    template<typename T>
    class Create : public Command {
    public:
      using Command::Command;

      void Execute() override {
        T object;
        if constexpr(std::is_same_v<T, regex::Regex>) {
          std::cin >> object;
        } else {
          object = automata::Parse<T>(std::cin);
        }
        cli_.AddObject(object);
      }
    };

    template<typename T>
    class OnObject : public Command {
    public:
      OnObject(CLI &cli, std::istream &args) : Command(cli, args) {
        object_ = &GetObject<T>();
      }

      T *object_;
    };

    class Print : public OnObject<Object> {
    public:
      using OnObject<Object>::OnObject;

      void Execute() override {
        std::visit([](auto &&object) { std::cout << object << std::endl; }, *object_);
      }
    };

    template<typename T>
    class AddState : public OnObject<T> {
    public:
      using OnObject<T>::OnObject;

      void Execute() override {
        this->object_->AddState();
      }
    };

    template<typename T>
    class AddTransition : public OnObject<T> {
    public:
      AddTransition(CLI &cli, std::istream &args) : OnObject<T>(cli, args) {
        args >> from_state >> to_state >> transition_string;
      }

      std::size_t from_state{}, to_state{};
      typename T::TransitionString transition_string;

      void Execute() override {
        this->object_->AddTransition(from_state, to_state, transition_string);
      }
    };

    template<typename T>
    class SetAccepting : public OnObject<T> {
    public:
      std::size_t state{};
      bool value;

      SetAccepting(CLI &cli, std::istream &args) : OnObject<T>(cli, args) {
        this->args_ >> state;
        if (!(this->args_ >> value)) {
          value = true;
        }
      }

      void Execute() override {
        this->automaton.SetAccepting(state, value);
      }
    };

    class Minimize : public OnObject<automata::DeterministicAutomaton> {
    public:
      using OnObject::OnObject;

      void Execute() override {
        cli_.AddObject(object_->Minimize());
      }
    };

    template<typename T>
    class WithAlphabet : public OnObject<T> {
    public:
      WithAlphabet(CLI &cli, std::istream &args) : OnObject<T>(cli, args) {
        std::string alphabet;
        args >> alphabet;
        std::ranges::copy(alphabet, std::back_inserter(alphabet_));
      }

    protected:
      std::vector<char> alphabet_;
    };

    class ToComplete : public WithAlphabet<automata::DeterministicAutomaton> {
    public:
      ToComplete(CLI &cli, std::istream &args) : WithAlphabet<automata::DeterministicAutomaton>(cli, args) {}

      void Execute() override {
        auto copy = *object_;
        cli_.AddObject(copy.MakeComplete(alphabet_));
      }
    };

    class Determinize : public OnObject<automata::NondeterministicAutomaton> {
    public:
      using OnObject::OnObject;

      void Execute() override {
        cli_.AddObject(object_->Determinize());
      }
    };

    class Complement : public WithAlphabet<Object> {
    public:
      Complement(CLI &cli, std::istream &args) : WithAlphabet<Object>(cli, args) {}

      void Execute() override {
        std::visit([this](auto &&object) {
          using T = std::decay_t<decltype(object)>;
          if constexpr(std::is_same_v<T, regex::Regex>) {
            this->cli_.AddObject(automata::RegexComplement(object, alphabet_));
          } else if constexpr(std::is_same_v<T, automata::DeterministicAutomaton>) {
            this->cli_.AddObject(object.ToComplete(alphabet_).Complement());
          }
        }, *this->object_);
      }
    };

    class ToRegex : public OnObject<automata::NondeterministicAutomaton> {
    public:
      using OnObject::OnObject;

      void Execute() override {
        cli_.AddObject(object_->ToRegex());
      }
    };

    class ToNFA : public OnObject<Object> {
    public:
      using OnObject::OnObject;

      void Execute() override {
        cli_.AddObject(ToNfa(*object_));
      }
    };

    class ToMCDFA : public WithAlphabet<regex::Regex> {
    public:
      ToMCDFA(CLI &cli, std::istream &args) : WithAlphabet<regex::Regex>(cli, args) {}

      void Execute() override {
        cli_.AddObject(automata::RegexToMCDFA(*object_, alphabet_));
      }
    };

    template<typename T, typename Q = T>
    class OnObjects : public Command {
    public:
      OnObjects(CLI &cli, std::istream &args) : Command(cli, args) {
        first_ = &GetObject<T>();
        second_ = &GetObject<Q>();
      }

      T *first_;
      Q *second_;
    };

    class Intersection : public OnObjects<Object> {
    public:
      using OnObjects::OnObjects;

      void Execute() override {
        this->cli_.AddObject(ToDfa(*first_).Intersection(ToDfa(*second_)));
      }
    };

    class Equivalence : public OnObjects<Object> {
    public:
      using OnObjects::OnObjects;

      void Execute() override {
        if (!ToDfa(*first_).IsEquivalent(ToDfa(*second_)))
          std::cout << "not ";
        std::cout << "equivalent" << std::endl;
      }
    };

    class AbstractCommandHandle {
    public:
      virtual ~AbstractCommandHandle() = default;

      explicit AbstractCommandHandle(std::string name) : name_(std::move(name)) {}

      virtual std::unique_ptr<Command> create(CLI &cli, std::istream &args) = 0;

      std::string name_;
    };

    template<typename T>
    class CommandHandle : public AbstractCommandHandle {
    public:
      using AbstractCommandHandle::AbstractCommandHandle;

      std::unique_ptr<Command> create(CLI &cli, std::istream &args) override {
        return std::make_unique<T>(cli, args);
      }
    };
  }

  template<typename T>
  void CLI::AddCommandHandle(std::string name) {
    command_handles.push_back(std::make_unique<command::CommandHandle<T>>(name));
  }
}

#endif //AUTOMATA_CLI_H
