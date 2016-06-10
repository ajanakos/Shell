#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

extern char ** get_args();

// Loop though args and grab a given operator
// I store the amount of arguments, the operator, the position of the operator
// Set flag if user used the '&' 
void setOperator(char **args, char **operator, int *position, int *len_args, int *amp_flag) {
    *operator = NULL;
    *position = -1;
    *len_args = 0;
    for (int i = 0; args[i] != NULL; i++) {

	if (!args[i+1])
	*len_args = i+1;

        if (!(strcmp(args[i], "<"))) {
            *operator = args[i];
            *position = i;
        }
        else if (!strcmp(args[i], ">")) {
            *operator = args[i];
            *position = i;
        }
	else if (!strcmp(args[i], ">>")) {
	    *operator = args[i];
	    *position = i;	    
	}
	else if (!strcmp(args[i], "|")) {
	    *operator = args[i];
	    *position = i;
	}
        else if (!strcmp(args[i], ">&")) {
            *operator = ">";
            *position = i;
            *amp_flag = 1;
        }
        else if (!strcmp(args[i], ">>&")) {
            *operator = ">>";
            *position = i;
            *amp_flag = 1;
        }
        else if (!strcmp(args[i], "|&")) {
            *operator = "|";
            *position = i;
            *amp_flag = 1;
        }
	else if (!strcmp(args[i], "cd")) {
	    *operator = args[i];
	    *position = i;
	}
    }
}
// Set left and right commands
void setCommands(char **args, int pos, char **lCmd, char **rCmd, int len_args) {
    
    // If operator exists grab args to left and right of operator position
    if (pos > 0) {
        for (int i = 0; i < pos; i++) {
	    lCmd[i] =  args[i];
	    lCmd[i+1] = NULL;
        }
        for (int i = 0; args[pos+i+1] != NULL; i++) {
	    rCmd[i] = args[pos+i+1];
	    rCmd[i+1] = NULL;
        }
    }
    
    // If no operator exists simply set the left commands as all args in array
    else {
        for (int i = 0; args[i] != NULL; i++) {
	    lCmd[i] = args[i];
	    lCmd[i+1] = NULL;
	}
	rCmd[0] = NULL;
	if (len_args == 0)
	    lCmd[0] = NULL;
    }
}

// Spawns child process to write or read given data
void execPipe(char *lCmd[], char *rCmd[], int pfd[], int amp_flag) {
    int pid;
    
    // Spawn child
    switch (pid = fork()) {
		
	// In child shell, Read from command inout
	case 0:
	    dup2(pfd[0], STDIN_FILENO);
	    close(pfd[1]);	
			
	    // Execute command
	    execvp(rCmd[0], rCmd);
	    perror(rCmd[0]);

	    // In parent shell, read from command output
	    default:
		dup2(pfd[1], STDOUT_FILENO);
			
		// Duplicate STDOUT onto STDERR so both get directed to file
		if (amp_flag)
		    dup2(STDOUT_FILENO, STDERR_FILENO);
		close(pfd[0]);	
			
		// Execute command
		execvp(lCmd[0], lCmd);
		perror(lCmd[0]);
    }
    exit(0);
}

// In case of no operator simply execute left command
int noOpCmd(char *lCmd[], char *rCmd[]) {
    execvp(lCmd[0], lCmd);
    perror(lCmd[0]);
    exit(0);
    return 0;	
}

// Redirect input from a specified file
int redirect_input(char *lCmd[], char *rCmd[]) {
	
    // Open file for read and write
    int fd = open(rCmd[0], O_RDWR);
	
    // Check that file can be opened
    if (fd == -1) {
 	fprintf(stderr, "%s\n", "Unable to open file");
	exit(0);
    }
	
    // Copy files descriptor over STDIN
    dup2(fd, STDIN_FILENO);
    close(fd);
	
    // Execute command
    execvp(lCmd[0], lCmd);
    perror(lCmd[0]);
	
    exit(0);
    return 0;
}

// Redirect output of command to file
int redirect_output(char *lCmd[], char *rCmd[], int amp_flag) {
	
    // Open file with flags enabling read/write, file replacement and creation
    int fd = open(rCmd[0], O_RDWR | O_TRUNC | O_CREAT, 0777);
   
    // Make sure file exists
    if (fd == -1) {
        fprintf(stderr, "%s\n", "Unable to open file");
   	exit(0);
    }
   	
    // Copy files descriptor over STDOUT
    // Lets commands output be directed to file
    dup2(fd, STDOUT_FILENO);
   	
    // Enable amp flag
    // Copy STDIN onto STDERR so both get directed to file
    if (amp_flag)
        dup2(STDOUT_FILENO, STDERR_FILENO);
    close(fd);
   	
    // Execute command
    execvp(lCmd[0], lCmd);
    perror(lCmd[0]);
   	
    exit(0);
    return 0;
}


// Redirect output of command to file
int redirect_output_append(char *lCmd[], char *rCmd[], int amp_flag) {

    // Open file with flags enabling read/write, file append and creation	
    int fd = open(rCmd[0], O_RDWR | O_APPEND | O_CREAT, 0777);
   	
    // Make sure file exists
    if (fd == -1) {
	fprintf(stderr, "%s\n", "Unable to open file");
	exit(0);
    }
   	
    // Copy files descriptor onto STDOUT
    // Lets commands output be directed to file
    dup2(fd, STDOUT_FILENO);
	
    // Set amp flag
    // Copy STDIN onto STDERR so both get directed to file
    if (amp_flag)
	dup2(STDOUT_FILENO, STDERR_FILENO);
    close(fd);
	
    // Execute command
    execvp(lCmd[0], lCmd);
    perror(lCmd[0]);
	
    exit(0);
    return 0;
}

int changeDir(char *arg, int len_args, char buffer[]) {


    // cd command by itself, move to initial shell spawn dir
    if (len_args==1) {
	int cd = chdir(buffer);
    }
    // Correct number args
    else if (len_args==2) {
		
        // Attempt cd
        int cd = chdir(arg);
		
        // Let us know in not successful
        if (cd != 0) {
	    fprintf(stderr, "%s\n", "Unable to change directory");
        }
    }
    // Incorrect number of arguments
    else {
	fprintf(stderr, "%s\n", "please speciify two arguments");
    }
    return 0;
}

int main()
{	
    // Process Variables
    pid_t pid = -1;
    int status;

    // Store user entered arguments
    char **arguments;
    char *args[256];
    
    // Int array for pipe
    int pfd[2];

    // Operator position and String val
    int pos;
    int len_args;
    int count = 0;
    char *op;
    int amp_flag;
    
    // Stores initial directory in which the shell was activated
    char buffer[1024];

    // User commands
    char *lCmd[1024];
    char *rCmd[1024];
    
    // Set initial working directory for cd command
    getcwd(buffer, sizeof(buffer));
    
    //fprintf(stderr, "%s", buffer );

    // Inside an infinite loop we continuously take user entered command
    // then execute the commands entered. 
    // We create a temp array args from arguments and execute the temp array
    // until arguments is exhausted
    while (1) { 
    	fprintf(stdout, "%s", "myShell: ");
    
    	// Initialize user arguments and other vars
    	arguments = get_args();
	    count = 0;
	    amp_flag = 0;
		
	// Loop through each argument entered
    	for (int x = 0; arguments[x] != NULL; x++) {
	    	
	    // Store in temp array if argument != ";"
	    if (strcmp(arguments[x], ";"))	
	        args[count++] = arguments[x];

	    // If argument == ";", execute temp array of args
	    if (!strcmp(arguments[x], ";") || !arguments[x+1]) {     

		// Get the operator associated with the command 
		setOperator(args, &op, &pos, &len_args, &amp_flag);
			
		// Get commands before and after operator
		setCommands(args, pos, lCmd, rCmd, len_args);
			
		// Check is first arg is 0 and exit
		if ( !strcmp (args[0], "exit")) {
                    printf ("Exiting...\n");
                    exit(1);
		}
				
		// Detect cd command 
		if (op) {
		    if (!strcmp(op, "cd")) {
		        changeDir(args[1], len_args, buffer);
			continue;
			}
	  	}		
				
		// Spawn child process
		pid = fork();
		// If inside child process
		if (pid == 0) {
				
		    // Depending on what the operator is execute the appropriate function
		    if (!op) {
		        noOpCmd(lCmd, rCmd);
	 	    }	
		    else if (!strcmp(op, "<")) {
		    	redirect_input(lCmd, rCmd);
		    }
		    else if (!strcmp(op, ">")) {
			redirect_output(lCmd, rCmd, amp_flag);	
		    }
		    else if (!strcmp(op, ">>")) {
			redirect_output_append(lCmd, rCmd, amp_flag);
		    }
		    else if (!strcmp(op, "|")) {
		    	// Spawn child to handle pipe
			pid = fork();
  						
  			// Create pipe
  			pipe(pfd);
               			
               		// If we are inside the child process execute the pipe
               		if (pid==0) {
	    		    execPipe(lCmd, rCmd, pfd, amp_flag);
               		}
               		else { 
               		    if (pid > 0)
               			wait(&status);
               		}
               		exit(0);
		    }
		}
				
		// Kill off child process
		else if (pid > 0) {
		    wait(&status);
		}
				
		// Let us know we couldn't fork()
		else {
		    fprintf(stderr,"%s\n", "Unable to fork");
		}
		
		// Reset args/temp array to NULL for next iteration
		for (int k = 0; args[k] != NULL; k++) {
		    args[k] = NULL;
		}
		count = 0;
	    }
	}
    }
}
