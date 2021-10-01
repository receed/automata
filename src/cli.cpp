#include "cli.h"

namespace cli {
  CLI::CLI() {
    AddCommandHandle<command::Create<regex::Regex>>("regex");
    AddCommandHandle<command::Create<automata::NondeterministicAutomaton>>("automaton");
    AddCommandHandle<command::AddState<automata::NondeterministicAutomaton>>("add_state");
    AddCommandHandle<command::AddTransition<automata::NondeterministicAutomaton>>("add_transition");
    AddCommandHandle<command::Print>("print");
  }

  std::size_t CLI::AddObject(cli::Object object) {
    objects_.push_back(std::move(object));
    return objects_.size() - 1;
  }

  void CLI::ExecuteCommand(const std::string &command_string) {
    std::istringstream stream(command_string);
    std::string command_name;
    stream >> command_name;
    for (const auto &command_handle : command_handles) {
      if (command_handle->name_ == command_name) {
        auto command = command_handle->create(*this, stream);
        command->Execute();
        return;
      }
    }
    std::cout << "Unknown command " << command_name << '\n';
  }

  void CLI::Start() {
    std::string line;
    while (std::getline(std::cin, line))
      ExecuteCommand(line);
  }
}