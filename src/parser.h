#ifndef MINI_SHELL_PARSER_H
#define MINI_SHELL_PARSER_H

#include <string>
#include <vector>

struct Command {
    std::vector<std::string> arguments;
    std::string inputFile;
    std::string outputFile;
    bool appendOutput = false;
};

struct Pipeline {
    std::vector<Command> commands;
    bool background = false;
};

bool parseCommandLine(const std::string& line, Pipeline& pipeline,
                      std::string& error);

#endif
