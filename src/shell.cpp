```cpp
#include "shell.h"

#include "executor.h"
#include "parser.h"

#include <bits/stdc++.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

// ------------------------------------------------------------
// Signal Handlers
// ------------------------------------------------------------

// Handles Ctrl+C (SIGINT)
void handleInterrupt(int) {
    const char newline = '\n';
    write(STDOUT_FILENO, &newline, 1);
}

// Handles changes in child processes
// Prevents zombie processes
void handleChildChange(int) {
    int savedErrno = errno;

    while (true) {
        pid_t childPid = waitpid(-1, nullptr, WNOHANG);

        // A child process was successfully reaped
        if (childPid > 0) {
            continue;
        }

        // waitpid was interrupted by another signal
        if (childPid == -1 && errno == EINTR) {
            continue;
        }

        // No more child processes to reap
        break;
    }

    // Restore errno
    errno = savedErrno;
}


// ------------------------------------------------------------
// Helper Functions
// ------------------------------------------------------------

// Displays the command history
void showHistory(const std::vector<std::string>& history) {

    for (std::size_t i = 0; i < history.size(); ++i) {
        std::cout << i + 1 << "  "
                  << history[i]
                  << '\n';
    }
}


// Checks whether a command uses input/output redirection
bool hasRedirection(const Command& command) {

    return !command.inputFile.empty() ||
           !command.outputFile.empty();
}


// ------------------------------------------------------------
// Signal Setup
// ------------------------------------------------------------

bool setupSignals() {

    // Setup Ctrl+C (SIGINT)
    struct sigaction interruptAction {};

    interruptAction.sa_handler = handleInterrupt;
    sigemptyset(&interruptAction.sa_mask);
    interruptAction.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &interruptAction, nullptr) == -1) {
        perror("sigaction");
        return false;
    }


    // Setup child process handling (SIGCHLD)
    struct sigaction childAction {};

    childAction.sa_handler = handleChildChange;
    sigemptyset(&childAction.sa_mask);
    childAction.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &childAction, nullptr) == -1) {
        perror("sigaction");
        return false;
    }

    return true;
}

} // namespace


// ============================================================
// Shell::run()
// Main loop of the shell
// ============================================================

int Shell::run() {

    // Setup signal handlers
    if (!setupSignals()) {
        return 1;
    }


    // Stores the last 10 commands
    std::vector<std::string> history;

    std::string inputLine;


    // --------------------------------------------------------
    // Main Shell Loop
    // --------------------------------------------------------

    while (true) {

        // Display shell prompt
        std::cout << "mini-shell$ " << std::flush;


        // Read command from user
        if (!std::getline(std::cin, inputLine)) {
            std::cout << '\n';
            break;
        }


        // Ignore empty or whitespace-only commands
        if (inputLine.find_first_not_of(" \t") == std::string::npos) {
            continue;
        }


        // ----------------------------------------------------
        // Store Command in History
        // ----------------------------------------------------

        history.push_back(inputLine);

        // Keep only the latest 10 commands
        if (history.size() > 10) {
            history.erase(history.begin());
        }


        // ----------------------------------------------------
        // Parse Command
        // ----------------------------------------------------

        Pipeline pipeline;
        std::string errorMessage;

        if (!parseCommandLine(inputLine, pipeline, errorMessage)) {

            std::cerr << "mini-shell: "
                      << errorMessage
                      << '\n';

            continue;
        }


        // Nothing to execute
        if (pipeline.commands.empty()) {
            continue;
        }


        // First command in the pipeline
        const Command& command = pipeline.commands[0];


        // ----------------------------------------------------
        // Handle Simple Built-in Commands
        // ----------------------------------------------------

        bool isSimpleBuiltin =
            pipeline.commands.size() == 1 &&
            !pipeline.background &&
            isBuiltin(command.arguments[0]) &&
            !hasRedirection(command);


        if (isSimple5Builtin) {

            // Exit the shell
            if (command.arguments[0] == "exit") {
                break;
            }

            // Execute other built-in commands
            runBuiltin(command);

            continue;
        }


        // ----------------------------------------------------
        // Handle History Command
        // ----------------------------------------------------

        bool isHistoryCommand =
            pipeline.commands.size() == 1 &&
            command.arguments[0] == "history" &&
            !pipeline.background &&
            !hasRedirection(command);


        if (isHistoryCommand) {
            showHistory(history);
            continue;
        }


        // ----------------------------------------------------
        // Execute External Commands / Pipelines
        // ----------------------------------------------------

        executePipeline(pipeline);
    }


    return 0;
}
```
