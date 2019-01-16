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

#ifndef MISH
#define MISH

#include "parser.h"

#define UP 1
#define DOWN -1
#define HOME 0

static pid_t CHILDPIDS[MAXCOMMANDS];
static int NRCHILDREN;

/*
* description: Internal echo command. Writes a string to standard output. Will
* remove quotation marks (").
* param[in]: c - Command struct with the echo command. All command line
* arguments in argv[] will be printed.
*/
void executeEcho (command c);

/*
* description: Writes a string to stdout without quotation marks.
* param[in]: str - The string.
*/
void printWithoutQuotationMarks (char *str);

/*
* description: Internal cd command. Changes directory either up or down in the
* directory tree. Will change to home directory if no command line argument is
* given in c.
* param[in]: c - Command struct with the cd command. First command line
* argument (argv[1]) should be the string to requested directory.
*/
void executeCd (command c);

/*
* description: Checks if a command line contains any internal commands ("echo"
* or "cd")
* param[in]: nrComms - Number of commands.
* param[in]: comLine[] - The command line: an array containing commands.
* return: If command line contaings internal command; 1, else -1.
*/
int containsInternalCommands (int nrComms, command comLine[nrComms]);

/*
* description: Executes input commands if they are not "echo" or "cd". If two
* or more commands are created, a pipe will be created and linked between each
* command input and output. The first and/or last command input/output can
* also be redirected to a file.
* param[in]: nrComm - Number of commands.
* param[in]: comLine - Command array with commands.
*/
void executeExternalCommands (int nrComms, command comLine[nrComms]);

/*
* description: Executes a command (ideally from a child process) with execvp.
* param[in]: c - The command to be executed.
*/
void executeCommand (command c);

/*
* description: Open a number of pipes and store them in array pipes.
* param[in]: nrPipes - Number of pipes.
* param[in]: pipes[] - The array storing the pipes.
* return: If succesfull; 1, else -1.
*/
int openPipes(int nrPipes, int pipes[nrPipes][2]);

/*
* description: Close all pipes in the array pipes[]
* param[in]: nrPipes - Number of pipes.
* param[in]: pipes[] - The array containing the pipes to be closed.
*/
void closePipes(int nrPipes, int pipes[nrPipes][2]);

/*
* description: For all created child processes with PID's stored in children[],
* current process (ideally parent) will call waitpid() for each of them.
* param[in]: nrChildren - Number of children.
* param[in]: children[] - Array with PID's for the children.
*/
void waitForChildren(int nrChildren, pid_t children[nrChildren]);

/*
* description: Sends a signal with kill() to all childproceses. PID's for
* children should be stored in global var CHILDPIDS. Number of children should
* be stored in global var NRCHILDREN.
* param[in]: signo - The signal to be sent to each of the children.
*/
void terminateChildren (int signo);

#endif //MISH
