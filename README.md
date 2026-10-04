# Mini Linux Shell

Mini Linux Shell is a small, educational command interpreter written in C++.
It demonstrates how a Unix-like shell reads a command, starts processes, and
connects their input and output using Linux/POSIX system calls.

It is deliberately not a Bash replacement. Its parser handles simple,
unquoted words and a small set of operators so the implementation remains
readable and suitable for learning.

## Features

- Interactive `mini-shell$` prompt and command history (last 10 commands).
- External programs such as `ls`, `grep`, `cat`, and `sleep`.
- Built-ins: `cd`, `pwd`, `help`, `history`, and `exit`.
- Multiple command arguments.
- Input redirection (`<`), output redirection (`>`), and append (`>>`).
- Pipelines with two or more commands.
- Background execution with a trailing `&`.
- Ctrl+C leaves the shell running and interrupts foreground children.

## Architecture

```text
                  +----------------------+
                  | main.cpp             |
                  | starts Shell         |
                  +----------+-----------+
                             |
                  +----------v-----------+
                  | shell.cpp            |
                  | prompt, built-ins,   |
                  | history, signals     |
                  +-----+-----------+----+
                        |           |
             +----------v--+    +---v----------------+
             | parser.cpp  |    | executor.cpp       |
             | words and   |    | fork/exec, wait,   |
             | operators   |    | pipes, redirection|
             +-------------+    +--------------------+
```

## Technologies

- C++17 standard library
- Linux / WSL
- POSIX process, file-descriptor, and signal APIs

## POSIX system calls used

| Call | Use in this project |
| --- | --- |
| `fork()` | Creates a child process to run each external pipeline command. |
| `execvp()` | Replaces a child with the requested program, searching `PATH`. |
| `waitpid()` | Waits for foreground children; the SIGCHLD handler reaps completed children. |
| `pipe()` | Creates a pair of file descriptors connecting adjacent commands. |
| `dup2()` | Makes a pipe or redirected file become standard input/output. |
| `open()` | Opens the files used by input and output redirection. |
| `close()` | Closes descriptors no longer needed by a process. |
| `chdir()` | Changes the working directory for the shell's `cd` built-in. |
| `getcwd()` | Gets the current directory for `pwd`. |
| `sigaction()` | Sets shell and child signal behavior. |

### How process execution works

For a normal external command, the shell calls `fork()`. The child process
prepares any descriptors and calls `execvp()`. On success, `execvp()` replaces
the child's program image with the requested program; it does not return. The
parent calls `waitpid()` for a foreground command, so the prompt appears after
that command finishes. For `command &`, the parent skips that wait and prints a
new prompt.

### How pipes work

`pipe()` creates two descriptors: a read end and a write end. For each command
in a pipeline, the child uses `dup2()` to connect its standard input to the
previous pipe's read end and/or its standard output to the next pipe's write
end. Every process closes the original pipe descriptors after duplication.
The parent starts every pipeline process before waiting, which lets data flow
between the commands concurrently.

### How redirection works

For `< file`, the child opens the file read-only and uses `dup2()` to make it
file descriptor 0 (standard input). For `> file`, it opens or creates the file
and truncates it, then makes it descriptor 1 (standard output). `>>` uses
append mode instead of truncation. After `dup2()`, the extra descriptor is
closed; the command continues to use standard input/output as usual.

### File descriptors

A file descriptor is a small integer used by a process to refer to an open
file or communication endpoint. By convention, descriptor 0 is standard input,
1 is standard output, and 2 is standard error. A pipe end and an opened file
also have descriptors.

## Example commands

```text
mini-shell$ ls -la /tmp
mini-shell$ echo hello world
mini-shell$ cd src
mini-shell$ pwd
mini-shell$ ls | grep cpp
mini-shell$ cat notes.txt | grep hello
mini-shell$ echo hello > greeting.txt
mini-shell$ echo again >> greeting.txt
mini-shell$ wc -l < greeting.txt
mini-shell$ sleep 10 &
mini-shell$ history
mini-shell$ help
mini-shell$ exit
```

## Compile

On Linux or WSL, from the project directory:

```sh
make
```

Equivalent direct compiler command:

```sh
g++ -std=c++17 -Wall -Wextra -pedantic src/main.cpp src/shell.cpp \
    src/parser.cpp src/executor.cpp -o mini-shell
```

## Run

```sh
./mini-shell
```

Type `exit` or press Ctrl+D to leave the shell.

## Example output

```text
$ ./mini-shell
mini-shell$ echo hello world
hello world
mini-shell$ pwd
/home/student/mini-linux-shell
mini-shell$ echo hello | grep ell
hello
mini-shell$ sleep 2 &
[background process started]
mini-shell$ history
1  echo hello world
2  pwd
3  echo hello | grep ell
4  sleep 2 &
5  history
mini-shell$ exit
```

## Important design detail: `cd`

`cd` must run in the shell process itself. The current working directory is
part of each process's state. If the shell forked a child and that child called
`chdir()`, only the child's directory would change; the parent shell would stay
where it was. Therefore, a simple standalone `cd` is handled before creating a
child. When a built-in is used in a pipeline or with redirection, it runs in a
child and cannot change the interactive shell's directory.

## Signal handling

The interactive shell ignores the default terminating behavior for SIGINT
(Ctrl+C), while its children restore the default SIGINT action before running a
command. The terminal delivers Ctrl+C to the foreground process group, so the
foreground child is interrupted while the shell remains available. A SIGCHLD
handler reaps exited children so they do not remain zombies. If the handler
reaps a foreground child before the parent's `waitpid()` call, that `waitpid()`
reports `ECHILD`, which the executor treats as already completed.

## Limitations

- No quotes or escaped spaces; arguments are split on spaces and tabs.
- No variable expansion, globbing, command substitution, or Bash grammar.
- Redirection accepts one input and one output file per command.
- `&` is supported only at the end of a whole command line.
- No job control (`fg`, `bg`, or job listing).
- No separate process groups; background commands do not have full job-control
  signal isolation.
- No command sequencing with `;`, conditional execution, or subshells.
- History is in memory only and is not saved after exiting.
- No exit-status display or advanced terminal editing.
- A built-in in a pipeline/redirection runs in a child, so its effects (such as
  `cd`) do not alter the interactive shell. `history` is only a standalone
  shell command.

## Possible future improvements

- Add quote-aware parsing and escaped characters.
- Add parser-focused unit tests and richer syntax error messages.
- Display command exit statuses and improve terminal line editing.
- Add a small job table and basic `jobs` support.
- Persist command history between runs.

## Interview Explanation

### Learn the project in 2–3 hours

1. **Start with the data structures.** `Command` holds one command's argument
   list and optional input/output files. `Pipeline` holds one or more commands
   and whether the complete pipeline runs in the background.
2. **Trace parsing.** `parseCommandLine()` first splits a line into words and
   operator tokens. It then groups words into commands separated by `|` and
   records redirection and background operators.
3. **Trace a normal command.** The shell recognizes standalone built-ins.
   Otherwise, `executePipeline()` creates any needed pipes and forks a child.
   That child connects descriptors, applies file redirection, and calls
   `execvp()`. The parent waits for foreground children.
4. **Trace a pipeline.** Each child gets the appropriate pipe ends through
   `dup2()`. All children are started before the parent waits.
5. **Trace shell-specific behavior.** `cd` runs in the parent so its directory
   change persists. SIGINT is ignored by the shell but restored to its default
   action in children. SIGCHLD reaps completed children.

### Interview questions and answers

**What problem does this project solve?**  
It demonstrates the core mechanics behind an interactive command shell:
parsing commands, starting processes, connecting streams, and managing child
processes.

**How does the shell execute a command?**  
It parses the input, forks a child, configures any redirection or pipe
descriptors, calls `execvp()` in the child, and waits in the parent unless the
command is in the background.

**What does `fork()` do?**  
It creates a new process by duplicating the calling process. The child and
parent continue from the point where `fork()` returned, with different return
values.

**What does `execvp()` do?**  
It replaces the calling process's program with another program. The `v` means
arguments are passed as a vector, and `p` means it searches the `PATH`.

**Why do we need `waitpid()`?**  
It lets the parent wait for a particular child to finish and collect its
completion, avoiding zombies for foreground processes.

**What is the difference between a parent and child process here?**  
The parent is the shell and remains available to read commands. A child runs a
requested program; `execvp()` replaces the child's program without replacing
the shell.

**How does `cd` work?**  
The shell calls `chdir()` in its own process. With no directory argument it
uses `HOME`.

**Why can't `cd` simply be executed using `execvp()`?**  
Changing directory in a child changes only the child's working directory. When
it exits, the parent shell is still in its original directory.

**How does a pipe work?**  
`pipe()` creates a read descriptor and a write descriptor. The producer writes
to the pipe, and the consumer reads from it; `dup2()` connects them to standard
output and standard input.

**What does `dup2()` do?**  
It makes one descriptor refer to the same open resource as another descriptor,
closing the destination first if needed. This is how a file or pipe becomes
standard input/output.

**What are file descriptors?**  
They are small integer handles a process uses for open files and I/O endpoints.
Standard input, output, and error are descriptors 0, 1, and 2.

**How does `>` redirection work?**  
The child opens/creates the destination with truncation, then uses `dup2()` to
connect it to standard output before executing the command.

**How does `<` redirection work?**  
The child opens the source file read-only and uses `dup2()` to connect it to
standard input.

**How does background execution using `&` work?**  
The shell forks the command but does not wait for it. It immediately returns
to the prompt; SIGCHLD handling reaps the process after it finishes.

**How does Ctrl+C work?**  
The shell installs a SIGINT handler that keeps the shell alive. Children reset
SIGINT to its default action, so the terminal's Ctrl+C interrupts foreground
commands.

**What happens when a child process finishes?**  
The parent waits for foreground children with `waitpid()`. The SIGCHLD handler
can reap a child first, in which case the wait code recognizes `ECHILD` as
already collected. It also reaps completed background children.

**What limitations does this shell have compared with Bash?**  
It has no quoting, expansion, globbing, job control, shell scripting grammar,
or persistent history, and supports only simple pipelines and redirections.
#   m i n i _ s h e l l  
 