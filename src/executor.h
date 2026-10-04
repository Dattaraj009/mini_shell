#ifndef MINI_SHELL_EXECUTOR_H
#define MINI_SHELL_EXECUTOR_H

#include "parser.h"

#include <string>

bool isBuiltin(const std::string& name);
int runBuiltin(const Command& command);
void executePipeline(const Pipeline& pipeline);

#endif
