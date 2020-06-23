/*
* mish - A shell program that can take multiple commands and redirect their
* stdin/stdout. Commands are separeted by pipes "|". The first and last command
* can also have their stdin/stdout redirected to files with the "<" and ">"
* operators.
*
* Mish have two internal commands - cd and echo. These commands can not be
* redirected to files and they cannot be linked with pipes.
*
* Mish do not take any arguments. To end mish, write "exit", "quit" or send an
* EOF (^D) to stdin.
*
* Author: Buster Hultgren Wärn <dv17bhn@cs.umu.se>
*
* Final build: 2018-10-09
*
* Modified by: Buster Hultgren Wärn
* Date:	2018-11-06
* What?	Added exit(1) calls for all systemcall checks.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

#include "parser.h"
#include "execute.h"
#include "mish.h"
#include "sighant.h"

int main (void) {

	NRCHILDREN = 0;
	signalHandler(SIGINT, terminateChildren);

	int nrComms = 0;
	char buffer[MAXWORDS];
	command comLine[MAXCOMMANDS + 1];

	fprintf(stderr, "mish%% ");
	fflush(stderr);
	while (fgets(buffer, MAXWORDS, stdin) != NULL &&
		   strncmp(buffer, "exit", 4) != 0 &&
		   strncmp(buffer, "quit", 4) != 0) {

		if (buffer[strlen(buffer) - 1] == '\n') {

			buffer[strlen(buffer) - 1] = '\0';
		}
		nrComms = parse(buffer, comLine);

		if (containsInternalCommands(nrComms, comLine) == 0) {

			executeExternalCommands(nrComms, comLine);
		} else if (strncmp(comLine[0].argv[0], "echo", 4) == 0 &&
				   nrComms == 1) {

			executeEcho(comLine[0]);
		} else if (strncmp(comLine[0].argv[0], "cd", 2) == 0 && nrComms == 1) {

			executeCd(comLine[0]);
		} else {

			fprintf(stderr, "Invalid input - echo and / or cd cannot be"
			" piped\n");
		}
		fprintf(stderr, "mish%% ");
		fflush(stderr);
	}
	fprintf(stderr, "\n");
	return errno;
}

/*
* description: Internal echo command. Writes a string to standard output. Will
* remove quotation marks (").
* param[in]: c - Command struct with the echo command. All command line
* arguments in argv[] will be printed.
*/
void executeEcho (command c) {

	for (int i = 1; i < c.argc; i++) {

		printWithoutQuotationMarks(c.argv[i]);
		if (i < c.argc - 1) {

			printf(" ");
		} else if (i == c.argc - 1) {

			printf("\n");
		}
	}
}

/*
* description: Writes a string to stdout without quotation marks.
* param[in]: str - The string.
*/
void printWithoutQuotationMarks (char *str) {

	int usedBackSlash = 0;
	for (int i = 0; str[i] != '\0'; i++) {

		if (str[i] == '"' && usedBackSlash == 1) {

			printf("%c", str[i]);
		} else if (usedBackSlash == 1) {

			usedBackSlash = 0;
		} else if (str[i] == '\\') {

			usedBackSlash = 1;
		} else if (str[i] != '"') {

			printf("%c", str[i]);
		}
	}
}

/*
* description: Internal cd command. Changes directory either up or down in the
* directory tree. Will change to home directory if no command line argument is
* given in c.
* param[in]: c - Command struct with the cd command. First command line
* argument (argv[1]) should be the string to requested directory.
*/
void executeCd (command c) {

	if (c.argc > 1) {

		if (chdir(c.argv[1]) < 0) {

			fprintf(stderr, "%s - ", c.argv[0]);
			perror("cd");
		}
	} else if (c.argc == 1) {

		if (chdir(getenv("HOME")) < 0) {

			perror("cd");
		}
	}
}

/*
* description: Checks if a command line contains any internal commands ("echo"
* or "cd")
* param[in]: nrComms - Number of commands.
* param[in]: comLine[] - The command line: an array containing commands.
* return: If command line contaings internal command; 1, else -1.
*/
int containsInternalCommands (int nrComms, command comLine[nrComms]) {

	int internalCommands = 0;
	for (int i = 0; i < nrComms && internalCommands == 0; i++) {

		if (strncmp(comLine[i].argv[0], "echo", 4) == 0 ||
			strncmp(comLine[i].argv[0], "cd", 2) == 0) {

				internalCommands = 1;
			}
	}
	return internalCommands;
}

/*
* description: Executes input commands if they are not "echo" or "cd". If two
* or more commands are created, a pipe will be created and linked between each
* command input and output. The first and/or last command input/output can
* also be redirected to a file.
* param[in]: nrComm - Number of commands.
* param[in]: comLine - Command array with commands.
*/
void executeExternalCommands (int nrComms, command comLine[nrComms]) {

	int nrPipes = nrComms - 1;
	int pipes[nrPipes - 1][2];
	int parent = openPipes(nrPipes, pipes);
	pid_t children[nrComms];

	for (int i = 0; i < nrComms && parent == 1; i++) {

		children[i] = fork();

		if (children[i] < 0 ) {					// FAILED FORK

			perror("fork");
			exit(1);
		} else if (children[i] > 0) {			// PARENT

			CHILDPIDS[NRCHILDREN] = children[i];
			NRCHILDREN++;
		} else {								// CHILD

			parent = 0;

			if (i < nrPipes) {

				dupPipe(pipes[i], WRITE_END, STDOUT_FILENO);
			}
			if (i > 0) {

				dupPipe(pipes[i - 1], READ_END, STDIN_FILENO);
			}
			if (i == nrComms - 1 && comLine[i].outfile != NULL) {

				redirect(comLine[i].outfile, WRITE_END, STDOUT_FILENO);
			}
			if (i == 0 && comLine[i].infile != NULL) {

				redirect(comLine[i].infile, READ_END, STDIN_FILENO);
			}

			closePipes(nrPipes, pipes);
			executeCommand(comLine[i]);
		}
	}

	if (parent == 1) {

		closePipes(nrPipes, pipes);
		waitForChildren(nrComms, children);
		NRCHILDREN = 0;
	}
}

/*
* description: Executes a command (ideally from a child process) with execvp.
* param[in]: c - The command to be executed.
*/
void executeCommand (command c) {

	if (execvp(c.argv[0], c.argv) < 0) {

		fprintf(stderr, "Invalid command - ");
		perror(c.argv[0]);
		exit(1);
	}
}

/*
* description: Open a number of pipes and store them in array pipes.
* param[in]: nrPipes - Number of pipes.
* param[in]: pipes[] - The array storing the pipes.
* return: If succesfull; 1, else -1.
*/
int openPipes(int nrPipes, int pipes[nrPipes][2]) {

	int noError = 1;
	for (int i = 0; i < nrPipes && noError > 0; i++) {

		if (pipe(pipes[i]) < 0) {

			perror("Opening parent pipes");
			exit(1);
		}
	}
	return noError;
}

/*
* description: Close all pipes in the array pipes[]
* param[in]: nrPipes - Number of pipes.
* param[in]: pipes[] - The array containing the pipes to be closed.
*/
void closePipes (int nrPipes, int pipes[nrPipes][2]) {

	for (int i = 0; i < nrPipes; i++) {

		if (close(pipes[i][READ_END]) < 0) {

			perror("Closing pipe's READ_END");
			exit(1);
		}
		if (close(pipes[i][WRITE_END]) < 0) {

			perror("Closing pipe's WRITE_END");
			exit(1);
		}
	}
}

/*
* description: For all created child processes with PID's stored in children[],
* current process (ideally parent) will call waitpid() for each of them.
* param[in]: nrChildren - Number of children.
* param[in]: children[] - Array with PID's for the children.
*/
void waitForChildren (int nrChildren, pid_t children[nrChildren]) {

	for (int i = 0; i < nrChildren; i++) {

		if (waitpid(children[i], NULL, 0) < 0) {

			perror("waitpid");
			exit(1);
		}
	}
}

/*
* description: Sends a signal with kill() to all childproceses. PID's for
* children should be stored in global var CHILDPIDS. Number of children should
* be stored in global var NRCHILDREN.
* param[in]: signo - The signal to be sent to each of the children.
*/
void terminateChildren (int signo)  {

	for (int i = 0; i < NRCHILDREN; i++) {

		if (kill(CHILDPIDS[i], signo) < 0) {

			perror("Terminating child process");
			exit(1);
		}
	}
	fprintf(stderr, "\n");
}
