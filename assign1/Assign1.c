/*
 ============================================================================
 Name        : Assign1.c
 Author      : Duan Li 260683698
 Description : Simple shell
 ============================================================================
 */
// clang assign1.c -g
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>


#define HISTORY_SIZE 10
#define ARGS_ARRAY_SIZE 20
#define JOBS_ARRAY_SIZE 20

typedef struct history_t{
	char* buffer[HISTORY_SIZE][ARGS_ARRAY_SIZE];
	int currentCmd;
} History;

typedef struct joblist_t{
	char* jobs[JOBS_ARRAY_SIZE][ARGS_ARRAY_SIZE];
	pid_t pids[JOBS_ARRAY_SIZE];
	int currJob;
} JobList;




int getcmd(History *hist, JobList *jobList, char *prompt, char *args[], int *background, int* builtInCmd);
int parseCommand(char *line, char *args[]);
int ifBackground(char *line, int *background);
int execCommand(char *args[], int background, JobList* jobList);
int addJob(char* args[], pid_t pid, char* jobs[][ARGS_ARRAY_SIZE], pid_t pids[JOBS_ARRAY_SIZE], int* currJob);
int addToHistory(char* buf[][ARGS_ARRAY_SIZE], int* currCmd, char *args[]);
int getHistoryIndex(char* args[]);
int execHistoryItem(char* buf[][ARGS_ARRAY_SIZE], int* currCmd,int index, char* args[]);
int tenpower(int i);	//this is a helper method to calculate the power of ten
int checkExit(char *args[]);
int checkpwd(char *args[]);
int checkJobs(char *args[], JobList* jobList);
void showJobs(char* jobs[][ARGS_ARRAY_SIZE], pid_t pids[], int currJob);


//===========================Other Built in commands==============================
/**
 * input:
 * 		char *args[]: the array of pointers to the words of input command
 * output:
 * 		int : 1 -- the user enters jobs
 * 			  0 -- the user doesnt't enter jobs
 * description:
 * 		this method checks if the user call jobs. If does, show jobs.
 */
int checkJobs(char *args[], JobList* jobList){
	char target[] = "jobs";

	if (strcmp(target, args[0]) == 0){
		//show jobs
		showJobs(jobList->jobs, jobList->pids, jobList->currJob);
		return 1;
	}
	return 0;
}

/**
 * input:
 * 		char* jobs[][ARGS_ARRAY_SIZE]: the pointers to the jobs args.
 * 		pid_t pids[JOBS_ARRAY_SIZE]: the pids of the jobs
 * 		int currJob: the index of the current job
 * description:
 * 		this method shows the jobs
 *
 */
void showJobs(char* jobs[][ARGS_ARRAY_SIZE], pid_t pids[], int currJob){
	int end = (currJob-1)%20;
	int i, j;

	for (i = 0; i<=end; i++){
		printf("%d	%d	", i, pids[i]);
		j = 0;
		while (jobs[i][j]!=NULL){
			printf("%s", jobs[i][j++]);
		}
		printf("\n");
	}
	return;
}

/**
 * input:
 * 		char* args[]: the array of pointers to the words of input command
 * output:
 * 		int : 1 -- the user enters pwd
 * 			  0 -- the user doesn't enter pwd
 * description:
 * 		this method checks if the user call pwd, if does, show the present working directory
 */

int checkpwd(char* args[]){
	char target[] = "pwd";

	if (strcmp(target, args[0]) == 0){
		char *buf = NULL;
		size_t len = 0;
		printf("%s", getcwd(buf, len));
		free(buf);
		return 1;
	}
	else return 0;
}



/**
 * input:
 * 		char* args[]: the array of pointers to the words of input command
 * output:
 * 		int : 1 -- the user wants to exit
 * 			  0 -- the user doesn't want to exit
 * description:
 * 		this method checks if the user wants to exit, and if he wants, exit the shell
 */
int checkExit(char* args[]){
	char target[] = "exit";

	if (strcmp(target, args[0]) == 0){
		exit(1);
	}
	else return 0;
};





//===========================History Part==========================================
/**
 * input:
 * 		int i: the power of ten
 * output:
 * 		int: the result
 * description:
 * 		this calculate i power 10;
 */
int tenpower(int i){
	int result = 1;
	while (i>0){
		result *= 10;
		i--;
	}
	return result;
}


/**
 * input:
 * 		char* args[]: the command args
 * output:
 * 		int: the index of the history command
 * 			 -1 -- user doesn't want to execute a history command
 * description:
 * 		this method detects whether the user want to execute a history command and return the index of the history command
 *
 */


int getHistoryIndex(char* args[]){
	char* c = args[0];

	//user doesn't want to execute a history command
	if ((*c)!=33) return -1;
	//user want to execute a history command, compute the index of the command
	int index = 0;
	c++;
	int i = strlen(c)-1;
	while ((*c)!='\0'){
		index += ((*c)-'0')*tenpower(i--);
		c++;
	}
	return index-1;
}

/**
 * input:
 * 		char* buf[][ARGS_ARRAY_SIZE]: buffer of the History
 * 		int* currCmd: current commend number
 * 		int index: index of the command user want to execute
 * 		char* args[]: the current command pointers
 * output:
 * 		int: 0-- if command is not found
 * 			 1-- SUCCESS
 * description:
 * 		this method gets nth command in the history buffer and loads it to the current argument (args) for execution (it doesn't execute the command), and add the command to the next entry in history buffer
 *
 */
int execHistoryItem(char* buf[][ARGS_ARRAY_SIZE], int* currCmd,int index, char* args[]){
	//invalid index
	if (index<(*currCmd)-10 || index>=*currCmd){
		printf("No command found in history.");
		return 0;
	}

	index = index % 10;
	int i = 0;

	//load the command to current command for execution
	while (buf[index][i]!=NULL){
		args[i] = buf[index][i++];
	}
	//add null;
	args[i] = buf[index][i];


	//print the command out
	i=0;
	while (args[i]!=NULL){
		printf("%s ", args[i++]);
	}
	//WHY print after the execution of command????Why if I put a puts here, it works???
	puts("\n");

	//puts("HEY");

	//add it history
	addToHistory(buf, currCmd, buf[index]);
	return 1;



}

/**
 * input:
 * 		History hist: the history of commands
 * 		char *args[]: the array of pointers that points to words of the current command
 * output:
 * 		int: the index of the current command
 * description:
 * 		this method add the current command into history record
 */
int addToHistory(char* buf[][ARGS_ARRAY_SIZE], int* currCmd, char *args[]){
	int i;
	int index = (*currCmd) % 10;
	for(i = 0; i<ARGS_ARRAY_SIZE; i++){
		buf[index][i] = args[i];
	}

	(*currCmd)++;
	return *currCmd;
}

//===================================================================================

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
int getcmd(History *hist, JobList *jobList, char *prompt, char *args[], int *background, int* builtInCmd){


	size_t len = 0;		//the size in bytes of the buffer to read the line
	ssize_t read;		//the number of character read
	char *line = NULL;	//the place to store the input command
	int cnt;
	int histIndex;

	//prints prompt
	printf("%s", prompt);

	//read input
	read = getline(&line, &len, stdin);		//getline does the memory allocation for line

	if (read == -1) exit(-1);		//check if the user want to exit (^D)
	if (read == 1) return 0;	//if there is no input (normally there will be '\n' so at least one character), return 0
	else{
		ifBackground(line, background);
		cnt = parseCommand(line, args);

		//built in commands
		checkExit(args);
		*builtInCmd+=checkpwd(args);
		*builtInCmd+=checkJobs(args, jobList);

		histIndex = getHistoryIndex(args);
		if (histIndex == -1){
			addToHistory((hist->buffer), &(hist->currentCmd),args);
		}
		else execHistoryItem(hist->buffer, &(hist->currentCmd), histIndex, args);


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

	args[cnt]=NULL;
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



/**
 * input :
 * 		pid_t pid : the pid of the job
 * 		char* jobs[][ARGS_ARRAY_SIZE]: the pointers to the jobs args.
 * 		pid_t pids[JOBS_ARRAY_SIZE]: the pids of the jobs
 * 		int* currJob: the index of the current job
 * 		char* args[]: the current command
 * output:
 * 		int: the index of the next job
 * description:
 * 		This method adds job into jobList.
 */
 int addJob(char* args[], pid_t pid, char* jobs[][ARGS_ARRAY_SIZE], pid_t pids[JOBS_ARRAY_SIZE], int* currJob){
	 int index = (*currJob)%20;
	 int i;
	 pids[index] = pid;

	 for(i = 0; i<ARGS_ARRAY_SIZE; i++){
		jobs[index][i] = args[i];
	 }

	 (*currJob)++;
	 printf("Added job %d.", *currJob);
	 return *currJob;
 }


/**
 * input :
 * 			char *args[] : the command
 * 			int *background : whether the new process should run in background
 * 			JobList* jobList: the list of jobs that runs in background
 * output :
 * 			int : 1 -- SUCCESS
 * 				  0 -- FAILURE
 * description:
 * 			execCommand fork a child process to run the command.
 */
int execCommand(char* args[], int background, JobList* jobList){
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
			addJob(args, pid, jobList->jobs, jobList->pids, &(jobList->currJob));
		}

	}
	return 1;
}


int main(void){
	char *args[20];		//args is the place to hold the command
	int bg;				//if child runs in background
	int builtInCmd;
	History *hist = (History*) malloc(sizeof(History));
	JobList *jobList = (JobList*) malloc(sizeof(JobList));

	while(1){		//while 1 loop
		bg = 0;
		builtInCmd = 0;
		int cnt =getcmd(hist, jobList, "\n>> ", args, &bg, &builtInCmd);
		if (cnt==-1) {
			printf("no command");
			exit(-1);
		}
		if (builtInCmd == 0) execCommand(args, bg, jobList);
	}

}
