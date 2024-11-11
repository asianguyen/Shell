#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "./jobs.h"

#define BUFSIZE 1024

char *redirection_array[4];
int foreground = 1;
int background = 0;

/**
 * This function parses the command line and stores each "argument" into an
 * array.
 *
 * buffer - contains the entire string to be parsed
 * tokens - each element of this array is a tokenized string broken by
 * whitespace/tab/newline argv - same as tokens, but stores a command's path to
 * the shortened version after the last /
 * returns - the path to the user's command
 */
void parse(char buffer[BUFSIZE], char *tokens[512], char *argv[512],
           char *bg[1]) {
    char *str = buffer;
    char *token;
    int index = 0;
    int argv_index = 0;
    while ((token = strtok(str, " \t\n")) != NULL &&
           (index < 511)) {  // runs until there's nothing left to be parsed or
                             // the array is out of space
        tokens[index] = token;
        if ((strcmp(token, "&")) != 0) {
            argv[argv_index] = token;
            argv_index++;
        }
        index++;
        str = NULL;
    }
    argv[argv_index] = NULL;
    bg[0] = tokens[index -
                   1];  // puts the last thing the user inputted into an array
}
/**
 * This function is a helper function that returns a number based on the
 * redirection symbol.
 *
 * tokens - the array that holds the user input
 * i - the index of the element to check
 */
int redirection_helper(char *tokens[], int i) {
    char *mychar = tokens[i];
    if ((strcmp(mychar, ">>")) == 0) {
        return 3;
    }
    switch (*mychar) {
        case '<':
            return 1;
            break;
        case '>':
            return 2;
            break;
    }
    return 0;
}

/**
 * This function handles the functionality of redirecting depending on the
 * redirection symbol.
 *
 * redirection_array - the array that holds the redirection symbols and their
 * corresponding files
 */
void redirection(char *redirection_array[]) {
    int i = 0, symbol_count_input = 0, symbol_count_output = 0;
    while (
        redirection_array[i] !=
        NULL) {  // runs until there aren't any redirection arguments to handle
        switch (redirection_helper(redirection_array, i)) {
            case 1:
                symbol_count_input++;
                close(0);
                int fdin = open(redirection_array[i + 1], O_RDONLY, 0);
                if (fdin == -1) {
                    perror("file open");
                    close(fdin);
                    exit(1);
                }
                break;
            case 2:
                symbol_count_output++;
                close(1);
                int fdout = open(redirection_array[i + 1],
                                 O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (fdout == -1) {
                    perror("file open");
                    close(fdout);
                    exit(1);
                }
                break;
            case 3:
                symbol_count_output++;
                close(1);
                int fdappend = open(redirection_array[i + 1],
                                    O_WRONLY | O_CREAT | O_APPEND, 0666);
                if (fdappend == -1) {
                    perror("file open");
                    close(fdappend);
                    exit(1);
                }
                break;
        }
        i++;
        i++;
    }
    if (symbol_count_input > 1 || symbol_count_output > 1) {
        fprintf(stderr, "Redirection syntax is incorrect\n");
    }
}

/**
 * This function parses the tokens array and places any redirection symbols and
 * their corresponding files into an array.
 *
 * tokens - the array that holds user input
 * argv - the array that holds user input, but no redirection symbols or their
 * corresponding files
 */
char *redirect_argv(char *tokens[512], char *argv[512]) {
    int j = 0, i = 0, k = 0;
    char *null = '\0';
    char *path;
    while (tokens[i] != null) {  // runs until the end of the user input tokens
        if ((strcmp(tokens[i], "<")) == 0 || (strcmp(tokens[i], ">")) == 0 ||
            (strcmp(tokens[i], ">>")) == 0) {
            redirection_array[k] = tokens[i];
            redirection_array[k + 1] = tokens[i + 1];
            k = k + 2;
            if (tokens[i + 2] != null &&
                (redirection_helper(tokens, i + 2)) ==
                    0) {  // checks to see if there is still user input after
                          // redirection
                argv[j] = argv[i + 2];
                i = i + 3;
                j++;
            } else {
                i++;
                i++;
            }
        } else {
            argv[j] = argv[i];
            j++;
            i++;
        }
    }
    path = argv[0];  // gets the full path to pass into execv
    if (argv[0] != NULL) {
        char *temp = strrchr(argv[0], '/') + 1;
        if (temp != NULL) {
            argv[0] = temp;
        }
    }

    for (int a = j; a < (int)sizeof(*argv); a++) {
        argv[a] = '\0';
    }
    return path;
}

/**
 * This function prints the prompt based on the PROMPT macro.
 */
void prompt_generator(void) {
#ifdef PROMPT
    const char *prompt = "asia's-shell> ";
    const char *error = "Attempt to produce prompt failed\n";
    if ((write(0, prompt, strlen(prompt))) < 0) {
        write(1, error, strlen(error));
    }
#endif
}

/**
 * This function handles the cd command.
 *
 * tokens - array that holds user input
 * argv - array that holds user input except for redirection symbols and their
 * corresponding files
 */
void cd(char *tokens[512], char *argv[512]) {
    if (argv[1] == NULL || argv[2] != NULL) {
        fprintf(stderr, "Attempt to cd failed\n");
    } else {
        if (chdir(tokens[1]) < 0) {
            perror("cd");
        }
    }
}

/**
 * This function handles the rm command.
 *
 * tokens - array that holds user input
 */
void rm(char *tokens[512]) {
    if (unlink(tokens[1]) < 0) {
        fprintf(stderr, "Attempt to rm failed\n");
    }
}

/**
 * This function handles the ln command.
 *
 * tokens - array that holds user input
 */
void ln(char *tokens[512]) {
    if (link(tokens[1], tokens[2]) < 0) {
        fprintf(stderr, "Attempt to ln failed\n");
    }
}

/**
 * This function reaps the background jobs based on their status.
 *
 * job_list - the list that holds the background jobs
 */
void reaper(job_list_t *job_list) {
    int status;
    pid_t current_pid;
    while ((current_pid = get_next_pid(job_list)) !=
           -1) {  // loops through list of background jobs
        int current =
            waitpid(current_pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
        if (current > 0) {
            int jid;
            if ((jid = get_job_jid(job_list, current_pid)) < 0) {
                fprintf(stderr, "Failed to get jid\n");
            }
            if (WIFSIGNALED(status)) {  // checks if the process was terminated
                                        // by a signal
                if ((remove_job_jid(job_list, jid)) < 0) {
                    fprintf(stderr, "Failed to remove job\n");
                } else {
                    fprintf(stdout, "[%d] (%d) terminated by signal %d\n", jid,
                            current_pid, WTERMSIG(status));
                    background--;
                }
            } else if (WIFSTOPPED(status)) {  // checks if the process was
                                              // stopped by a signal
                if ((update_job_jid(job_list, jid, STOPPED)) < 0) {
                    fprintf(stderr, "Failed to update job\n");
                } else {
                    fprintf(stdout, "[%d] (%d) suspended by signal %d\n", jid,
                            current_pid, WSTOPSIG(status));
                }
            } else if (WIFCONTINUED(
                           status)) {  // checks if the process was resumed
                if ((update_job_jid(job_list, jid, RUNNING)) < 0) {
                    fprintf(stderr, "Failed to update job\n");
                } else {
                    fprintf(stdout, "[%d] (%d) resumed\n", jid, current_pid);
                }
            } else if (WIFEXITED(status)) {  // checks if the process was
                                             // terminated normally
                if ((remove_job_jid(job_list, jid)) < 0) {
                    fprintf(stderr, "Failed to remove job\n");
                } else {
                    fprintf(stdout,
                            "[%d] (%d) terminated with exit status %d\n", jid,
                            current_pid, WEXITSTATUS(status));
                    background--;
                }
            }
        }
    }
}

/**
 * This function handles the built-in command fg.
 *
 * job_list - the list that holds the background jobs
 * argv - array that holds user input except for redirection symbols and their
 * corresponding files
 */
void fg_command(job_list_t *job_list, char *argv[]) {
    int status;
    if (background >= 0) {
        int jid = atoi((strrchr(argv[1], '%')) +
                       1);  // gets the jid from the user input
        int pid = get_job_pid(job_list, jid);
        if (pid > 0) {
            tcsetpgrp(STDIN_FILENO, pid);
            kill(-pid, SIGCONT);
            waitpid(pid, &status, WUNTRACED);
            if (WIFSIGNALED(status)) {  // checks to see if the process was
                                        // terminated by a signal
                if ((remove_job_jid(job_list, jid)) < 0) {
                    fprintf(stderr, "Failed to remove job\n");
                } else {
                    fprintf(stdout, "(%d) terminated by signal %d\n", pid,
                            WTERMSIG(status));
                    background--;
                }
            } else if (WIFSTOPPED(status)) {  // checks to see if the process
                                              // was suspended by a signal
                if ((update_job_jid(job_list, jid, STOPPED)) < 0) {
                    fprintf(stderr, "Failed to update job\n");
                } else {
                    fprintf(stdout, "[%d] (%d) suspended by signal %d\n", jid,
                            pid, WSTOPSIG(status));
                }
            } else {
                remove_job_jid(job_list, jid);
            }
            tcsetpgrp(STDIN_FILENO, getpgrp());
            foreground++;
        } else if (pid == -1) {
            fprintf(stdout, "job not found\n");
        }
    }
}

/**
 * This function handles the bg command.
 *
 * job_list - the list that holds the background jobs
 * argv - array that holds user input except for redirection symbols and their
 * corresponding files
 */
void bg_command(job_list_t *job_list, char *argv[]) {
    int jid = atoi((strrchr(argv[1], '%')) + 1);
    int pid = get_job_pid(job_list, jid);
    if (pid == -1) {
        fprintf(stdout, "job not found\n");
    } else {
        kill(-pid, SIGCONT);
        update_job_jid(job_list, jid, RUNNING);
        foreground--;
        background++;
    }
}

/**
 * Where the shell program is handled.
 */
int main(void) {
    char buf[BUFSIZE];
    char *tokens[512];
    char *argv[512];
    char *bg[1];
    char *path;
    int count;
    int jid = 1;
    pid_t pid;
    job_list_t *job_list = init_job_list();
    prompt_generator();
    while ((count = (int)read(0, buf, sizeof(buf))) >
           0) {  // runs as long as there is user input to read
        if (signal(SIGINT, SIG_IGN) == SIG_ERR ||
            signal(SIGTSTP, SIG_IGN) == SIG_ERR ||
            signal(SIGTTOU, SIG_IGN) == SIG_ERR) {
            perror("Signal");
        }
        if (count == -1) {
            perror("read");
        } else if (count < (int)sizeof(buf)) {
            buf[count] = '\0';
            for (int i = count + 1; i < (int)sizeof(buf); i++) {
                buf[i] = '\0';
            }
            parse(buf, tokens, argv, bg);
            path = redirect_argv(tokens, argv);
        }
        if (argv[0] == NULL) {
        } else if (strcmp(tokens[0], "exit") == 0) {
            cleanup_job_list(job_list);
            exit(0);
        } else if (strcmp(tokens[0], "cd") == 0) {
            cd(tokens, argv);
            continue;
        } else if (strcmp(tokens[0], "rm") == 0) {
            rm(tokens);
            continue;
        } else if (strcmp(tokens[0], "ln") == 0) {
            ln(tokens);
            continue;
        } else if (strcmp(tokens[0], "jobs") == 0) {
            jobs(job_list);
            continue;
        } else if (strcmp(tokens[0], "fg") == 0) {
            fg_command(job_list, argv);
            continue;
        } else if (strcmp(tokens[0], "bg") == 0) {
            bg_command(job_list, argv);
            continue;
        } else if ((pid = fork()) == 0) {  // creates a child process for the
                                           // non-builtin commands to be ran
            setpgid(pid, 0);
            if (strcmp(bg[0], "&")) { //foreground
                tcsetpgrp(STDIN_FILENO, getpgrp());
            }
            if (signal(SIGINT, SIG_DFL) == SIG_ERR ||
                signal(SIGTSTP, SIG_DFL) == SIG_ERR ||
                signal(SIGTTOU, SIG_DFL) == SIG_ERR) {
                perror("Signal");
            }
            char *null = '\0';
            if (redirection_array[0] != null) {
                redirection(redirection_array);
            }
            execv(path, argv);
            perror("execv");
            exit(1);
        }
        if (argv[0] == NULL) {
        } else if (strcmp(bg[0], "&") == 0) {  // checks background processes
            if ((add_job(job_list, jid, pid, RUNNING, path)) == -1) {
                fprintf(stderr, "Failed to add job?\n");
            } else {
                fprintf(stdout, "[%d] (%d)\n", jid, pid);
                jid++;
                background++;
            }
        } else {
            int wstatus;  // checks foreground processes
            waitpid(pid, &wstatus, WUNTRACED);
            if (WIFSIGNALED(wstatus)) {  // checks to see if the foreground
                                         // process was terminated by a signal
                fprintf(stdout, "(%d) terminated by signal %d\n", pid,
                        WTERMSIG(wstatus));
                foreground--;
            } else if (WIFSTOPPED(
                           wstatus)) {  // checks to see if the foreground
                                        // process was suspended by a signal
                if ((add_job(job_list, jid, pid, STOPPED, path)) < 0) {
                    fprintf(stderr, "Failed to add job\n");
                } else {
                    fprintf(stdout, "[%d] (%d) suspended by signal %d\n", jid,
                            pid, WSTOPSIG(wstatus));
                    jid++;
                    background++;
                }
            }
            tcsetpgrp(STDIN_FILENO, getpgrp());
        }
        reaper(job_list);  // reaps the background jobs
        prompt_generator();
    }
    cleanup_job_list(job_list);
    return 0;
}
