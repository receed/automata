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

  class CLI {
  public:
    CLI();
    std::size_t AddObject(Object object);

    template<typename T>
    T &GetObject(std::size_t id) {
      if constexpr(std::is_same_v<T, Object>)
        return objects_.at(id);
      else {
        auto object = std::get_if<T>(&objects_.at(id));
        assert(object);
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
    };

    template<typename T>
    class Create : public Command {
    public:
      using Command::Command;

      void Execute() override {
        T object;
        if constexpr(std::is_same_v<T, regex::Regex>)
          std::cin >> object;
        else
          object = automata::Parse<T>(std::cin);
        std::size_t id = cli_.AddObject(object);
        std::cout << "Id: " << id << '\n';
      }
    };

    template<typename T>
    class OnObject : public Command {
    public:
      OnObject(CLI &cli, std::istream &args) : Command(cli, args) {
        std::size_t id;
        args_ >> id;
        object = &cli_.GetObject<T>(id);
      }

      T* object;
    };

    class Print : public OnObject<Object> {
    public:
      using OnObject<Object>::OnObject;

      void Execute() override {
        std::visit([](auto && object) { std::cout << object << std::endl; }, *object);
      }
    };

    template<typename T>
    class AddState : public OnObject<T> {
    public:
      using OnObject<T>::OnObject;

      void Execute() override {
        this->object->AddState();
      }
    };

    template<typename T>
    class AddTransition : public OnObject<T> {
    public:
      AddTransition(CLI &cli, std::istream &args) : OnObject<T>(cli, args) {
        args >> from_state >> to_state >> transition_string;
      }

      std::size_t from_state, to_state;
      typename T::TransitionString transition_string;

      void Execute() override {
        this->object->AddTransition(from_state, to_state, transition_string);
      }
    };

    template<typename T>
    class SetAccepting : public OnObject<T> {
    public:
      std::size_t state;
      bool value;

      SetAccepting(CLI &cli, std::istream &args) : OnObject<T>(cli, args) {
        this->args_ >> state;
        if (!(this->args_ >> value))
          value = true;
      }

      void Execute() override {
        this->automaton.SetAccepting(state, value);
      }
    };

    class AbstractCommandHandle {
    public:
      virtual ~AbstractCommandHandle() = default;

      AbstractCommandHandle(std::string name) : name_(std::move(name)) {}

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
