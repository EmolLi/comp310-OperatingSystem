#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


#define HISTORY_SIZE 10
#define ARGS_ARRAY_SIZE 20
#define JOBS_ARRAY_SIZE 20

typedef struct history_t{
	char* buffer[HISTORY_SIZE][ARGS_ARRAY_SIZE];
	int currentCmd;
} History;

typedef struct joblist_t{
	char jobs[JOBS_ARRAY_SIZE][ARGS_ARRAY_SIZE];
	pid_t pids[JOBS_ARRAY_SIZE];
} JobList;

History hist;
JobList jobList;


/*
	Question: linecap: line capacity?
	
*/
int getcmd(char *prompt, char *args[], int *background){
	int length, i = 0;
	char *token, *loc;
	char *line = NULL;
	size_t linecap = 0;

	printf("%s", prompt);

	if (length <= 0){
		exit(-1);
	}

	//check if background is specified
	if ((loc = index(line, '&'))!=NULL) {
		*background = 1;
		*loc = ' ';
	} else
		*background = 0;
	
	while ((token = strsep(&line, " \t\n")) != NULL){         //the parsing looks for space, change it to null character
		int j = 0;
		for (j = 0; j<strlen(token); j++)
			if (token[j] <= 32)
				token[j] = '\0';
			if (strlen(token) >0)
				args[i++] = token; 
	}
	printf("%s",token);
	return i;
}

int main(void){
	char *args[20];
	int bg;
	while(1){
		bg = 0;
		int cnt =getcmd("\n>> ", args, &bg);	//count
		args[cnt] = NULL;
		printf("%d", cnt);

		//fork a child
		pid_t pid =fork();

		if(pid == 0){ //child
			execvp(args[0], args); //execvp fooks for argument in args, until it meets a null pointer
			exit(0);
		}
		else{
			if (bg==0){

			//child's status
			int status;

			//pid is the pid of the process you want to wait for
			waitpid(pid, &status, 0);
			}
			else{
	//			add_jobg(pid, args);
			}
			}
	}
	
}
