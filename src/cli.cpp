#include "cli.h"

namespace cli {
  CLI::CLI() {
    AddCommandHandle<command::Create<regex::Regex>>("regex");
    AddCommandHandle<command::Create<automata::NondeterministicAutomaton>>("automaton");
    AddCommandHandle<command::AddState<automata::NondeterministicAutomaton>>("add_state");
    AddCommandHandle<command::AddTransition<automata::NondeterministicAutomaton>>("add_transition");
    AddCommandHandle<command::Print>("print");
    AddCommandHandle<command::Minimize>("minimize");
    AddCommandHandle<command::ToComplete>("to_complete");
    AddCommandHandle<command::Determinize>("determinize");
    AddCommandHandle<command::Complement>("complement");
    AddCommandHandle<command::Intersection>("intersection");
    AddCommandHandle<command::ToRegex>("to_regex");
    AddCommandHandle<command::ToNFA>("to_nfa");
    AddCommandHandle<command::ToMCDFA>("to_mcdfa");
  }

  std::size_t CLI::AddObject(cli::Object object) {
    std::size_t id = objects_.size();
    std::cout << "Id: " << id << '\n';
    objects_.push_back(std::move(object));
    return id;
  }
  void CLI::ExecuteCommand(const std::string &command_string) {
    std::istringstream stream(command_string);
    std::string command_name;
    stream >> command_name;
    for (const auto &command_handle: command_handles) {
      if (command_handle->name_ == command_name) {
        auto command = command_handle->create(*this, stream);
        command->Execute();
        return;
      }
    }
    std::cout << "Unknown command " << command_name;
  }

  void CLI::Start() {
    std::string line;
    while (std::getline(std::cin, line)) {
      try {
        ExecuteCommand(line);
      } catch (const InvalidInputException &e) {
        std::cout << "invalid input: " << e.what();
      } catch (const automata::BadAutomatonException &e) {
        std::cout << "automaton has a wrong type: " << e.what();
      } catch (const std::exception &e) {
        std::cout << e.what();
      } catch (...) {
        std::cout << "error";
      }
      std::cout << std::endl;
    }
  }
}