```cpp
#include "executor.h"

#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstdlib>

using namespace std;

// Close all pipe file descriptors
void closePipes(const vector<int>& pipes) {
    for (int fd : pipes) {
        close(fd);
    }
}

// Handle input/output redirection
bool redirectFiles(const Command& command) {

    // Input: command < input.txt
    if (!command.inputFile.empty()) {
        int fd = open(command.inputFile.c_str(), O_RDONLY);

        if (fd == -1) {
            perror("open");
            return false;
        }

        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    // Output: command > output.txt
    // Output append: command >> output.txt
    if (!command.outputFile.empty()) {

        int flags = O_WRONLY | O_CREAT;

        if (command.appendOutput)
            flags |= O_APPEND;
        else
            flags |= O_TRUNC;

        int fd = open(command.outputFile.c_str(), flags, 0644);

        if (fd == -1) {
            perror("open");
            return false;
        }

        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    return true;
}


// Execute one child process
void runChild(const Pipeline& pipeline,
              const vector<int>& pipes,
              size_t index) {

    const Command& command = pipeline.commands[index];

    // If this is not the first command,
    // take input from previous pipe.
    if (index > 0) {
        dup2(pipes[(index - 1) * 2], STDIN_FILENO);
    }

    // If this is not the last command,
    // send output to next pipe.
    if (index < pipeline.commands.size() - 1) {
        dup2(pipes[index * 2 + 1], STDOUT_FILENO);
    }

    // Child doesn't need any pipe descriptors anymore.
    closePipes(pipes);

    // Handle <, > and >>
    if (!redirectFiles(command)) {
        exit(1);
    }

    // Convert vector<string> to char* array
    vector<char*> args;

    for (const string& arg : command.arguments) {
        args.push_back(const_cast<char*>(arg.c_str()));
    }

    args.push_back(nullptr);

    // Replace child process with actual command
    execvp(args[0], args.data());

    // execvp only returns if it fails
    perror("execvp");
    exit(1);
}


// Wait for all child processes
void waitForChildren(const vector<pid_t>& children) {

    for (pid_t child : children) {
        waitpid(child, nullptr, 0);
    }
}


// Check whether command is a shell built-in
bool isBuiltin(const string& name) {

    return name == "cd" ||
           name == "pwd" ||
           name == "help" ||
           name == "exit";
}


// Execute built-in command
int runBuiltin(const Command& command) {

    const string& name = command.arguments[0];

    // cd
    if (name == "cd") {

        const char* home = getenv("HOME");

        string path;

        if (command.arguments.size() > 1)
            path = command.arguments[1];
        else if (home)
            path = home;

        if (path.empty()) {
            cerr << "HOME is not set\n";
            return 1;
        }

        if (chdir(path.c_str()) == -1) {
            perror("cd");
            return 1;
        }
    }

    // pwd
    else if (name == "pwd") {

        char directory[4096];

        if (getcwd(directory, sizeof(directory)) == nullptr) {
            perror("pwd");
            return 1;
        }

        cout << directory << '\n';
    }

    // help
    else if (name == "help") {

        cout << "Built-ins: cd, pwd, help, exit\n";
    }

    return 0;
}


// Main pipeline execution
void executePipeline(const Pipeline& pipeline) {

    if (pipeline.commands.empty())
        return;

    vector<int> pipes;

    // Create pipes
    // N commands need N-1 pipes.
    for (size_t i = 0; i < pipeline.commands.size() - 1; i++) {

        int pipefd[2];

        if (pipe(pipefd) == -1) {
            perror("pipe");
            return;
        }

        pipes.push_back(pipefd[0]); // read end
        pipes.push_back(pipefd[1]); // write end
    }

    vector<pid_t> children;

    // Create processes
    for (size_t i = 0; i < pipeline.commands.size(); i++) {

        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            return;
        }

        // Child
        if (pid == 0) {
            runChild(pipeline, pipes, i);
        }

        // Parent
        children.push_back(pid);
    }

    // Parent no longer needs pipes
    closePipes(pipes);

    // Background command
    if (pipeline.background) {

        cout << "[background process started]\n";
        return;
    }

    // Foreground command
    waitForChildren(children);
}
```
