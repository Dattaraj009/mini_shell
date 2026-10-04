#include "parser.h"

namespace {

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::string word;

    for (std::size_t i = 0; i < line.size(); ++i) {
        const char character = line[i];
        if (character == ' ' || character == '\t') {
            if (!word.empty()) {
                tokens.push_back(word);
                word.clear();
            }
        } else if (character == '|' || character == '<' ||
                   character == '>' || character == '&') {
            if (!word.empty()) {
                tokens.push_back(word);
                word.clear();
            }
            if (character == '>' && i + 1 < line.size() && line[i + 1] == '>') {
                tokens.push_back(">>");
                ++i;
            } else {
                tokens.push_back(std::string(1, character));
            }
        } else {
            word += character;
        }
    }

    if (!word.empty()) {
        tokens.push_back(word);
    }
    return tokens;
}

}

bool parseCommandLine(const std::string& line, Pipeline& pipeline,
                      std::string& error) {
    pipeline = Pipeline{};
    error.clear();
    const std::vector<std::string> tokens = tokenize(line);
    if (tokens.empty()) {
        return true;
    }

    if (tokens.back() == "&") {
        pipeline.background = true;
    }

    Command current;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const std::string& token = tokens[i];

        if (token == "&") {
            if (i != tokens.size() - 1) {
                error = "'&' is only supported at the end of a command";
                return false;
            }
            continue;
        }

        if (token == "|") {
            if (current.arguments.empty()) {
                error = "missing command before '|'";
                return false;
            }
            pipeline.commands.push_back(current);
            current = Command{};
            continue;
        }

        if (token == "<" || token == ">" || token == ">>") {
            if (i + 1 >= tokens.size() || tokens[i + 1] == "|" ||
                tokens[i + 1] == "<" || tokens[i + 1] == ">" ||
                tokens[i + 1] == ">>" || tokens[i + 1] == "&") {
                error = "redirection requires a file name";
                return false;
            }
            const std::string& file = tokens[++i];
            if (token == "<") {
                if (!current.inputFile.empty()) {
                    error = "only one input redirection is supported per command";
                    return false;
                }
                current.inputFile = file;
            } else {
                if (!current.outputFile.empty()) {
                    error = "only one output redirection is supported per command";
                    return false;
                }
                current.outputFile = file;
                current.appendOutput = token == ">>";
            }
            continue;
        }

        current.arguments.push_back(token);
    }

    if (current.arguments.empty()) {
        error = "missing command";
        return false;
    }
    pipeline.commands.push_back(current);
    return true;
}
