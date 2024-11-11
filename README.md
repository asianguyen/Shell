# Custom Shell Program

## Overview

This custom shell program simulates a simple command-line interface with advanced process management, file redirection, and error handling. It supports built-in commands, background and foreground job control, and handles user input and system errors effectively.

### Key Features

- **Command Parsing**:
  - The shell parses user input into three key arrays:
    - **`tokens`**: Holds tokenized strings of the user input.
    - **`argv`**: Contains the shortened command path (e.g., `echo`), along with its arguments (excluding redirection symbols and files).
    - **`redirection`**: Stores redirection symbols (`<`, `>`, `>>`) and their corresponding files.

- **Built-In Commands**:
  - The shell supports the following built-in commands:
    - **`cd`**: Changes the current working directory.
    - **`ln`**: Creates a hard link.
    - **`rm`**: Removes a file.
    - **`exit`**: Exits the shell.

- **Process Management**:
  - **Background Jobs**:
    - Users can run commands in the background by appending `&` to the command.
    - The shell tracks background jobs using a `job_list`, displaying the job ID and process ID.
    - The **`bg`** command resumes a suspended job and moves it to the background.
  - **Foreground Jobs**:
    - The shell waits for a child process to change state (terminated, suspended, etc.) and prints informative messages about the process.
    - The **`fg`** command brings a background job to the foreground, gives it terminal control, and waits for the process to complete.

- **Redirection**:
  - The shell supports input/output redirection with symbols such as `>`, `<`, and `>>`. It opens files as specified by the user and performs redirection accordingly.

- **Error Handling**:
  - **User Input Errors**: Informative messages (e.g., "Redirection syntax is incorrect") are displayed when invalid input is detected.
  - **System Call Errors**: The shell checks system call return values and uses `perror()` to print error messages when necessary.

- **Job Control**:
  - **Job List**:
    - The shell keeps track of all background jobs in a `job_list`, allowing users to monitor their status.
  - **`jobs` Command**:
    - Displays all current jobs with job IDs, process IDs, and their statuses.

- **Zombie Process Handling**:
  - The shell reaps zombie processes by calling `waitpid()` on background jobs and printing informative status messages based on the job's state.

---

