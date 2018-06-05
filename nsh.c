#include "parse.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
// execcmd takes the commandline struct as input
// and executes the commands that are parsed
int execcmd(struct commandLine * cmdLine);

/* NOTE: I used the code from MAIN-PARSE to begin my nsh.c 
** A very simple main program that re-prints the line after it's been scanned and parsed.
*/
int main(int argc, char *argv[])
{
    FILE *input;
    char line[MAX_LINE];
	
	// if a shell script is provided as an argument, open this file as input instead of stdin
    if(argc == 2)
    {
		input = fopen(argv[1], "r");
		if(input == NULL)
		{
			perror(argv[1]);
			exit(1);
		}
    }
    else
    {
		assert(argc == 1);
		input = stdin;
		printf("? ");
		/* By default, printf will not "flush" the output buffer until
		* a newline appears.  Since the prompt does not contain a newline
		* at the end, we have to explicitly flush the output buffer using
		* fflush.
		*/
		fflush(stdout);
    }

	// read input
    setlinebuf(input);
    while(fgets(line, sizeof(line), input) != NULL)
    {
		struct commandLine cmdLine;

		//memset(&cmdLine, 0, sizeof(cmdLine));
		if(line[strlen(line)-1] == '\n')
			line[strlen(line)-1] = '\0';   /* zap the newline */

		// parse the command and populate the cmdLine struct
		Parse(line, &cmdLine);

		// if its a built-in command such as cd, execute in the parent shell 
		if (cmdLine.numCommands > 0){
			if (strcmp(cmdLine.argv[0], "cd") == 0){
			// pass in the filepath to chdir
				const char * path = cmdLine.argv[1];
				if (path == NULL){
					path = getenv("HOME");
				}
				if (chdir(path) < 0){
					fprintf(stderr, "cannot cd into %s\n", path);
				}
			}
			// execute command
			else{
				execcmd(&cmdLine);
			}
		}
		
		
		
		if(input == stdin)
		{
			printf("? ");
			fflush(stdout);
		}
	}
	return 0;
}

int execcmd(struct commandLine * cmdLine){

	int i;
	int orig_in;
	int orig_out;
	if (cmdLine->infile){
				// save the original stdin to restore it later
				orig_in = dup(STDIN_FILENO);
				int fd_in = open(cmdLine->infile, O_RDONLY); 

				if (fd_in < 0){
					perror("error opening file");
					exit(-1);
				}
				// duplicate the file descriptor to be stdin, and close the file descriptor
				// that fd_in occupied previously
				dup2(fd_in, STDIN_FILENO);
				close(fd_in);
			
		
		
		//printf("%s %s\n", cmdLine->infile, cmdLine->argv[0]);
	}
	//int pid = fork();
	//// if it's the child, exec the command indicated by parse
	//if (pid == 0){
	//}
	
	if(cmdLine->outfile){
				int fd_out;
				// save the original stdout to restore it later
				orig_out = dup(STDOUT_FILENO);
				if(cmdLine->append){

					/* verify that if we're appending there should be an outfile! */
					assert(cmdLine->outfile);
					// open a file with additional append permissions
					fd_out = open(cmdLine->outfile, O_WRONLY | O_APPEND | O_CREAT, 0600); 
					
				}
				else{
					// 0644 denotes read and write permission
					fd_out = creat(cmdLine->outfile, 0644);

				}

				if (fd_out < 0){
					fprintf(stderr, "could not open file: %s\n", cmdLine->outfile);
					exit(-1);
				}
				// duplicate the file to stdout and close the previous fd that occupied the opened file
				// so the stdout is now the out file
				dup2(fd_out, STDOUT_FILENO);
				close(fd_out);
			
		
	}
	// if the number of commands is only one, don't use pipes
	if (cmdLine->numCommands == 1){
		int j;
		int status;
		int pid = fork();
		if (pid < 0){
			perror("fork error");
		}
		if (pid == 0){
			for (j = cmdLine -> cmdStart[0]; cmdLine->argv[j] != NULL; j++){
				if (execvp(cmdLine->argv[j], &(cmdLine->argv[j])) < 0){
					perror("failed to execute");
					exit(-1);
				}
			}
		}
		else{
			waitpid(pid, &status, 0);
		}
		
	}
	else{
		// This code accounts for one pipe only
		// 32 is the max number of arguments which Professor Hayes chose in 		
		// in his parser 
		char * left[32];
		char * right[32];
		memset(left, 0, sizeof(left));
		memset(right, 0, sizeof(right));
		int pipe_fd[2];

		// create pipe
		pipe(pipe_fd);
		int pid1;
		int pid2;
		int status1;
		int status2;
		for (i = 0; i < cmdLine->numCommands; i++){
			int j;
		    int counter	= 0;
			// find the left and right sides of the pipe 
			for (j = cmdLine->cmdStart[i]; cmdLine->argv[j] != NULL; j++){

				if (i == 0){
					left[counter] = cmdLine->argv[j];	
				}
				if (i == 1){
					right[counter] = cmdLine->argv[j];			
				}
				counter++;
			}
			if (i == 0){
				left[counter+1] = '\0';
			}
			if (i == 1){
				right[counter+1]= '\0';
			}
		}
		pid1 = fork();
		// implementation of pipe for left
		if (pid1 < 0){
			perror("fork");
		}
		if (pid1 == 0){

			// duplicate the stdout of the pipe to stdout 
			// and close the file descriptor that was previously pipe_fd[1]
			close(STDOUT_FILENO);
			dup(pipe_fd[1]);
			// close the stdin of the pipe
			close(pipe_fd[0]);
			close(pipe_fd[1]);
			if (execvp(left[0], left) < 0){
				perror("execvp");
				exit(-1);
			}
		}
		pid2 = fork();
		if (pid2 < 0){
			perror("fork");
		}
		if (pid2 == 0){

			// duplicate the stdin of the pipe to stdin
			// and close the file descriptor that was previously pipe_fd[0]
			close(STDIN_FILENO);
			dup(pipe_fd[0]);
		
			// close the stdout of the pipe since we aren't using it
			close(pipe_fd[0]);
			close(pipe_fd[1]);
			if (execvp(right[0], right) < 0){
				perror("execvp");
				exit(-1);
			}
		}
		// after child consumes the data, close both ends of the pipe
		// so the child doesn't keep reading, thinking there's more input when there isn't
		close(pipe_fd[0]);
		close(pipe_fd[1]);

		// wait for both children to finish
		waitpid(pid1,&status1, 0);
		waitpid(pid2,&status2,0);

	}
	if (cmdLine->outfile){

		// restore stdout 
		dup2(orig_out, STDOUT_FILENO);
		close(orig_out);
	}
	if (cmdLine->infile){
		// restore stdin
		dup2(orig_in, STDIN_FILENO);
		close(orig_in);
	}
	return 0;
}
