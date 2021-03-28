/*
 * Copyright 2015 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "support/command-line.h"
#include "config.h"
#include "support/debug.h"

using namespace wasm;

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 80
#endif

void printWrap(std::ostream& os, int leftPad, const std::string& content) {
  int len = content.size();
  int space = SCREEN_WIDTH - leftPad;
  std::string nextWord;
  std::string pad(leftPad, ' ');
  for (int i = 0; i <= len; ++i) {
    if (i != len && content[i] != ' ' && content[i] != '\n') {
      nextWord += content[i];
    } else {
      if (static_cast<int>(nextWord.size()) > space) {
        os << '\n' << pad;
        space = SCREEN_WIDTH - leftPad;
      }
      os << nextWord;
      space -= nextWord.size() + 1;
      if (space > 0) {
        os << ' ';
      }
      nextWord.clear();
      if (content[i] == '\n') {
        os << '\n';
        space = SCREEN_WIDTH - leftPad;
      }
    }
  }
}

Options::Options(const std::string& command, const std::string& description)
  : debug(false), positional(Arguments::Zero) {
  std::string GeneralOption = "General options";

  if (getenv("BINARYEN_DEBUG")) {
    setDebugEnabled(getenv("BINARYEN_DEBUG"));
  }

  add("--version",
      "",
      "Output version information and exit",
      GeneralOption,
      Arguments::Zero,
      [command](Options*, const std::string&) {
        std::cout << command << " version " << PROJECT_VERSION << '\n';
        exit(0);
      });
  add("--help",
      "-h",
      "Show this help message and exit",
      GeneralOption,
      Arguments::Zero,
      [this, command, description](Options* o, const std::string&) {
        for (size_t i = 0; i < SCREEN_WIDTH; i++) {
          std::cout << '=';
        }
        std::cout << '\n';
        std::cout << command;
        if (positional != Arguments::Zero) {
          std::cout << ' ' << positionalName;
        }
        std::cout << "\n\n";
        printWrap(std::cout, 0, description);
        std::cout << '\n';
        for (size_t i = 0; i < SCREEN_WIDTH; i++) {
          std::cout << '=';
        }
        std::cout << '\n';
        size_t optionWidth = 0;
        for (const auto& o : options) {
          if (o.hidden) {
            continue;
          }
          optionWidth =
            std::max(optionWidth, o.longName.size() + o.shortName.size());
        }
        for (int i = int(categories.size()) - 1; i >= 0; i--) {
          auto& category = categories[i];
          std::cout << "\n\n" << category << ":\n";
          for (size_t i = 0; i < category.size() + 1; i++) {
            std::cout << '-';
          }
          std::cout << '\n';
          for (const auto& o : options) {
            if (o.hidden || o.category != category) {
              continue;
            }
            std::cout << '\n';
            bool long_n_short =
              o.longName.size() != 0 && o.shortName.size() != 0;
            size_t pad =
              1 + optionWidth - o.longName.size() - o.shortName.size();
            std::cout << "  " << o.longName << (long_n_short ? ',' : ' ')
                      << o.shortName << std::string(pad, ' ');
            printWrap(std::cout, optionWidth + 4, o.description);
            std::cout << '\n';
          }
        }
        std::cout << '\n';
        exit(EXIT_SUCCESS);
      });
  add("--debug",
      "-d",
      "Print debug information to stderr",
      GeneralOption,
      Arguments::Optional,
      [&](Options* o, const std::string& arguments) {
        debug = true;
        setDebugEnabled(arguments.c_str());
      });
}

Options::~Options() {}

Options& Options::add(const std::string& longName,
                      const std::string& shortName,
                      const std::string& description,
                      const std::string& category,
                      Arguments arguments,
                      const Action& action,
                      bool hidden) {
  options.push_back(
    {longName, shortName, description, category, arguments, action, hidden, 0});

  if (std::find(categories.begin(), categories.end(), category) ==
      categories.end()) {
    categories.push_back(category);
  }

  return *this;
}

Options& Options::add_positional(const std::string& name,
                                 Arguments arguments,
                                 const Action& action) {
  positional = arguments;
  positionalName = name;
  positionalAction = action;
  return *this;
}

void Options::parse(int argc, const char* argv[]) {
  assert(argc > 0 && "expect at least program name as an argument");
  size_t positionalsSeen = 0;
  auto dashes = [](const std::string& s) {
    for (size_t i = 0; i < s.size(); ++i) {
      if (s[i] != '-') {
        return i;
      }
    }
    return s.size();
  };
  for (size_t i = 1, e = argc; i != e; ++i) {
    std::string currentOption = argv[i];

    // "-" alone is a positional option
    if (dashes(currentOption) == 0 || currentOption == "-") {
      // Positional.
      switch (positional) {
        case Arguments::Zero:
          // Optional arguments must use --flag=A format, and not separated by
          // spaces (which would be ambiguous).
        case Arguments::Optional:
          std::cerr << "Unexpected positional argument '" << currentOption
                    << "'\n";
          exit(EXIT_FAILURE);
        case Arguments::One:
          if (positionalsSeen) {
            std::cerr << "Unexpected second positional argument '"
                      << currentOption << "' for " << positionalNa