# Welcome to Quash ###by Prasun Dhungana
#### A simple command shell in C

## Quash Shell Implementation Report

### Introduction
In this project, I developed Quash, a custom command-line shell program in C that emulates fundamental shell functionalities. The objective was to create a robust, feature-rich shell that demonstrates key operating systems concepts such as process management, signal handling, and I/O redirection.

### System Design Overview

#### Architecture
The Quash shell is designed with a modular approach, centered around a main event loop that continuously:

- Displays the current working directory prompt
- Reads user input
- Parses and tokenizes commands
- Handles built-in commands
- Manages process execution for external commands
- Supports advanced features like background processes, signal handling, and I/O redirection

#### Key Components
The shell's implementation revolves around several critical functions:

- **main()**: The primary event loop controlling shell execution
- **handle_redirection_and_pipes()**: Manages complex command processing
- Signal handlers for `SIGINT` and `SIGALRM`

### Implementation Details

#### Built-in Commands
I implemented six essential built-in commands:

- **cd**: Change directory, with support for home directory and relative paths
- **pwd**: Print current working directory
- **echo**: Print messages and environment variables
- **setenv**: Set environment variables
- **env**: Display or retrieve environment variable values
- **exit**: Terminate the shell

#### Process Management
The shell supports multiple process execution models:

- Foreground process execution
- Background process execution (using `&`)
- Process timeout mechanism (10-second limit)

### Advanced Features

#### Signal Handling
I implemented two crucial signal handlers:

- **sigint_handler()**: Captures `SIGINT` (Ctrl+C) without terminating the shell
- **sigalrm_handler()**: Terminates long-running foreground processes after 10 seconds

#### I/O Redirection
The `handle_redirection_and_pipes()` function supports:

- Input redirection (`<`)
- Output redirection (`>`)
- Piping between commands (`|`)

### Challenges and Solutions

#### Environment Variable Parsing
- **Challenge**: Expanding environment variables dynamically
- **Solution**: In the echo command implementation, I used `getenv()` to retrieve variable values. This allows users to reference variables like `$HOME` or custom variables.

#### Process Timeout Mechanism
- **Challenge**: Preventing infinite-running processes
- **Solution**: Implemented a 10-second timeout using `SIGALRM` and `kill()`, ensuring responsive shell behavior.

#### Background Process Management
- **Challenge**: Allowing concurrent command execution
- **Solution**: Detect `&` at command line end, use `setsid()` to detach background processes from the terminal.

### Code Structure and Design Principles

#### Modular Design
I structured the code to separate concerns:

- Built-in command handling
- External command execution
- Signal and process management
- I/O redirection logic

#### Error Handling
Comprehensive error handling using:

- `perror()` for system call errors
- Informative error messages
- Graceful error recovery

#### Memory Management
Careful memory usage by:

- Using stack-allocated arrays
- Avoiding dynamic memory allocation
- Utilizing system calls efficiently

#### Performance Considerations
The shell is designed for low overhead:

- Minimal memory footprint
- Efficient process spawning
- Quick command parsing and execution

### Potential Improvements
While the current implementation is robust, potential enhancements include:

- More comprehensive built-in command support
- Advanced pipe chaining
- Improved environment variable manipulation
- Command history tracking

### Conclusion
Quash demonstrates a practical implementation of a Unix-like shell, showcasing process management, signal handling, and I/O redirection. The modular design allows for easy extension and provides a solid foundation for understanding operating system interactions.

### Technologies and Tools

- **Programming Language**: C
- **System Calls**: `fork()`, `execvp()`, `wait()`, `signal()`
- **Development Environment**: Unix/Linux
- **Compiler**: GCC
