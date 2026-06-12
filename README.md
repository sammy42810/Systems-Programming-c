# Systems Programming in C

A collection of systems programming projects in C, covering data structures, file system utilities, process management, IPC, and networked applications — all built against the POSIX API.

## Projects

### HW2 — Generic Binary Search Tree
A type-generic BST implementation using `void*` data pointers and function pointers for comparison and printing. Supports insertion, in-order traversal, and full tree destruction with no memory leaks.

```
bstree.h / bstree.c   # Generic BST with function pointer interface
main.c                # Driver for testing with multiple data types
utils.h / utils.c     # Comparison and print helpers
```

**Concepts:** Generic programming in C via `void*`, function pointers, recursive tree operations, manual memory management.

---

### HW3 — `pfind`: Recursive File Finder
A command-line tool similar to `find`, that recursively traverses a directory tree and prints files matching a given permission string (e.g., `rwxr-xr--`).

```bash
./pfind /some/directory -p rwxr-x---
```

Parses and validates a 9-character permission string, converts it to a `mode_t` bitmask using `stat()`, and recurses with `opendir`/`readdir`.

**Concepts:** `stat()`, `dirent.h`, Unix permission bitmasks, recursive directory traversal, `getopt`.

---

### HW4 — `minishell`: Mini Unix Shell
A functional Unix shell with a custom prompt, signal handling, built-in `cd`, and arbitrary command execution via `fork`/`exec`.

Features:
- Colored prompt showing current directory (`[username@path] $`)
- `Ctrl-C` cancels current input without exiting (custom `SIGINT` handler)
- Built-in `cd` with support for `~`, `~/path`, and relative/absolute paths
- Executes any external command by forking and calling `execvp`
- Waits for child processes and reports non-zero exit codes

**Concepts:** `fork`, `execvp`, `waitpid`, `signal`/`sigaction`, `getpwuid`, process lifecycle.

---

### HW5 — `sl`: Sorted Directory Listing with IPC
Lists and counts files in a directory by spawning two child processes (`ls` and `sort`) and piping their output together, then reading the final result from the parent.

```bash
./sl /some/directory
```

**Concepts:** `pipe()`, `fork`, `dup2`, `execlp`, inter-process communication, reading from child process output.

---

### Project — Multiplayer Network Quiz Game
A multiplayer trivia game over TCP. A central server handles multiple simultaneous clients using `select()` for I/O multiplexing — no threads.

Features:
- Server reads questions from a configurable file (`-f`), each with a prompt, three choices, and a correct answer
- Tracks per-player scores across all rounds and announces a winner
- Configurable IP, port, and question file via CLI flags (`-i`, `-p`, `-f`)

```bash
make
./server [-f questions.txt] [-i 127.0.0.1] [-p 25555]
./client [-i 127.0.0.1] [-p 25555]
```

**Concepts:** POSIX TCP sockets, `select()` for multiplexing multiple file descriptors, `getopt`, client-server protocol design.

---

## Tech Stack

- **Language:** C (C99)
- **APIs:** POSIX — `unistd.h`, `sys/socket.h`, `sys/stat.h`, `signal.h`, `dirent.h`
- **Build:** GNU Make
- **Platform:** Linux / Unix
