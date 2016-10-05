/*
 ============================================================================
 Name        : Assign1.c
 Author      : emol
 Version     :
 Copyright   : Your copyright efhehnotice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>




/*
	input:
		prompt: the prompt of the shell
		args[]: the place to load the input command to

	output:
		int i: 1 -- there is a command;
			   0 -- no command

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

	//read input
	read = getline(&line, &len, stdin);
	if (read == -1 || read == 1) return 0;	//if there is no input, return 0
	else{
		printf(line);
		return 1;
	}


}


/**
 * input:
 * 		char *line: the command user entered, it at least has one word
 * 		char *args[]: the place to stored the words of the line
 * output:
 * 		int: 1 -- success
 * 			 0 -- error
 * 	description:
 * 		parseCommand parses the command, splits it into words, and stores them into args[]
 */
int parseCommand(char *line, char *args[]){
	const char *delimiter = ' ';

	return 0;
}




int main(void){
	char *args[20];		//args is the place to hold the command
	int bg;				//if child runs in background
	while(1){		//while 1 loop
		bg = 0;
		int cnt =getcmd("\n>> ", args, &bg);

		printf("%d", cnt);
	}

}
