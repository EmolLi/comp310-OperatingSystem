/*
 ============================================================================
 Name        : Assign1.c
 Author      : emol
 Version     :
 Copyright   : Your copyright efhehnotice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
// clang assign1.c -g
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

int getcmd(char *prompt, char *args[], int *background);
int parseCommand(char *line, char *args[]);
int ifBackground(char *line, int *background);
int execCommand(char *args[], int background);
int addJob(pid_t pid);

/*
	input:
		prompt: the prompt of the shell
		args[]: the place to load the input command to, the stores pointer to words in line

	output:
		int cnt: count of the command;
-+
	description:
		1.getcmd outputs and prompt
		2.the user types commands
		3.getcmd reads the input,parse it
		4.getcmd loads the command to args[]
*/
int getcmd(char *prompt, char *args[], int *background){


	size_t len = 0;		//the size in bytes of the buffer to read the line
	ssize_t read;		//the number of character read
	char *line = NULL;	//the place to store the input command
	int cnt;

	//prints prompt
	printf("%s", prompt);

	//read input
	read = getline(&line, &len, stdin);		//getline does the memory allocation for line

	if (read == -1) exit(-1);		//check if the user want to exit (^D)
	if (read == 1) return 0;	//if there is no input (normally there will be '\n' so at least one character), return 0
	else{
		ifBackground(line, background);
		cnt = parseCommand(line, args);
		return cnt;
	}

}







/**
 * input:
 * 		char *line: the command user entered, it at least has one word
 * 		char *args[]: the place to stored the words of the line
 * output:
 * 		int cnt: the number of words
 * 	description:
 * 		parseCommand parses the command, splits it into words, and stores the pointers to each word to args
 */

 int parseCommand(char *line, char *args[]){
	char *token;

	int cnt = 0;
	while((token = strsep(&line, " \t\n"))!=NULL){
		int j;
		for (j=0; j<strlen(token); j++){
			if (token[j] <= 32){   //replace the space with '\0'
				token[j] = '\0';
			}
		}
		if (strlen(token) > 0) args[cnt++] = token;	//so this avoid having a bunch of spaces that looks like a lot of command
	}

	return cnt;
}


/**
 * input:
 * 		char *line : command
 * 		char *background: the pointer to the integer that specify whether the child process runs in background
 * output:
 * 		int: 1--run in background
 * 			 0--don't run in background
 * description:
 * 		the method finds if there is a '&' in the command (whether the child process needs to run in background
 */
 int ifBackground(char *line, int *background){
	 char *loc;

	 if ((loc=index(line,'&')) != NULL){
		 *background = 1;
		 *loc = ' ';
	 }
	 return *background;
 }




 int addJob(pid_t pid){
	 return 0;
 }


/**
 * input :
 * 			char *args[] : the command
 * 			int *background : whether the new process should run in background
 * output :
 * 			int : 1 -- SUCCESS
 * 				  0 -- FAILURE
 * description:
 * 			execCommand fork a child process to run the command.
 */
int execCommand(char* args[], int background){
	//fork a child
	pid_t pid =fork();

	if(pid == 0){ //child
		execvp(args[0], args); //execvp fooks for argument in args, until it meets a null pointer
		exit(0);
	}
	else{
		if (background == 0){
			//child does not run in background
			int status;
			waitpid(pid, &status, 0);
		}
		else{
			//run in background
			addJob(pid);
		}

	}
	return 1;
}


int main(void){
	char *args[20];		//args is the place to hold the command
	int bg;				//if child runs in background
	while(1){		//while 1 loop
		bg = 0;
		int cnt =getcmd("\n>> ", args, &bg);
		if (cnt==-1) exit(-1);
		args[cnt]=NULL;
		execCommand(args, bg);
	}

}
