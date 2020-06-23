#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "execute.h"

/* Duplicate a pipe to a standard I/O file descriptor
 * Arguments:	pip	the pipe
 *		end	tells which end of the pipe shold be dup'ed; it can be
 *			one of READ_END or WRITE_END
 *		destfd	the standard I/O file descriptor to be replaced
 * Returns:	-1 on error, else destfd
 */
int dupPipe(int pip[2], int end, int destfd) {

	if (!(end == READ_END || end == WRITE_END)) {

		fprintf(stderr, "dupPipe - Must dup to READ_END or WRITE_END\n");
		destfd = -1;
	} else {

		if ((end == READ_END && destfd == STDIN_FILENO) ||
			(end == WRITE_END && destfd == STDOUT_FILENO)) {

			destfd = dup2(pip[end], destfd);

			if (destfd < 0) {

				perror("Dup to pipe");
				exit(1);
			}
		} else {

			fprintf(stderr,"dupPipe - Invalid arguments\n");
			destfd = -1;
		}
	}
	return destfd;
}

/* Redirect a standard I/O file descriptor to a file
 * Arguments:	filename	the file to/from which the standard I/O file
 * 				descriptor should be redirected
 * 		flags	indicates whether the file should be opened for reading
 * 			or writing
 * 		destfd	the standard I/O file descriptor which shall be
 *			redirected
 * Returns:	-1 on error, else destfd
 */
int redirect(char *filename, int flags, int destfd) {

	if ((flags == O_RDONLY && destfd != STDIN_FILENO) ||
		(flags == O_WRONLY && destfd != STDOUT_FILENO)) {

			fprintf(stderr, "Redirect must be done with stdin with flag"
							" O_RDONLY or stdout with flag O_WRONLY\n");
			destfd = -1;
	} else if (destfd == STDIN_FILENO) {

		if (access(filename, F_OK) >= 0) {

			destfd = open(filename, O_RDONLY);

			if (destfd >= 0) {

				if (dup2(destfd, STDIN_FILENO) < 0) {

					perror("cannot dup stdin\n");
					exit(1);
				}
				close(destfd);
			} else {

				perror("redirect stdin");
				exit(1);
			}
		} else {

			fprintf(stderr, "file %s does not exist\n", filename);
			destfd = -1;
		}
	} else if (destfd == STDOUT_FILENO) {

		if (access(filename, F_OK) < 0) {

			destfd = open(filename, O_RDWR | O_CREAT,
						            S_IRUSR | S_IRGRP | S_IROTH);

			if (destfd >= 0) {

				if (dup2(destfd, STDOUT_FILENO) < 0) {

					perror("redirect stdout");
					exit(1);
				}
				close(destfd);
			} else {

				perror("redirect stdout");
				exit(1);
			}
		} else {

			fprintf(stderr, "file %s already exists\n", filename);
			destfd = -1;
		}
	}
	return destfd;
}
