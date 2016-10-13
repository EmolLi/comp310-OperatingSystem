#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */





// ======================= History Feature =======================

// initiate history buffer to store the commands
char history[10][MAX_LINE]; // 10 commands should be enough..
int historyLength = 0;

// save command to history
void pushToHistory(char *command){

    // shift every history record leftward by 1, if necessary
    if (history[10-1][0] != '\0'){
        for (int i=1; i<10; i++){
            strcpy(history[i-1], history[i]);
        }
    }
    // put command to the first empty spot
    for (int i=0; i<10; i++){
        if (history[i][0] == '\0') {
            strcpy(history[i], command);
            break;
        }
    }
    historyLength++;
}

int isSpecificHistory(char *input){ return (strcmp(input, "r")) == 0; }
int isLatestHistory(char *input){ return (strlen(input)>=3 && strncmp(input, "r ", 2)==0); }

// determine if user wants history
int isHistoryCommand(char *input){
    int is = 0;
    if (isSpecificHistory(input) || isLatestHistory(input)) is = 1;
    return is;
}

// determine if the history command is valid
int isValidHistory(char *input){
    int is = 0;
    if (historyLength == 0) is = 0; // if history is empty, mark it as invalid
    else if (isSpecificHistory(input)) is = 1;
    else if (isLatestHistory(input)){
        for (int i=10-1; i>=0; i--){
            if (input[2] == history[i][0]){
                is = 1;
                break;
            }
        }
    }
    return is;
}

// get a specific history
char * getHistory(char *input){
    char *command;
    if (isSpecificHistory(input)){ // latest command
        command = history[historyLength-1];
    }
    else if (isLatestHistory(input)){
        for (int i=10-1; i>=0; i--){
            if (input[2] == history[i][0]){
                command = history[i];
                break;
            }
        }
    }
    return command;
}








// ======================= Build-in Commands =======================

// display all history
void displayHistory(){
    if (historyLength == 0){
        printf("No history to be displayed.\n");
    }
    else {
        printf("Displaying history:\n");
        for (int i=0; i<10; i++){
            if (history[i][0] != '\0') {
                printf("%i %s\n", i+1, history[i]);
            }
        }
    }
}

char jobs[100][1000]; // job buffer is able to hold 100 background jobs
int jobCount = 0;

// display all background jobs
void displayJobs(){
    int count = 0;
    printf("Displaying jobs:\n");
    for (int i=0; i<=100; i++){
        if (jobs[i][0] != '\0') {
            printf("%s\n", jobs[i]);
            count ++;
        }
    }
    if (count == 0){
        printf("No backgrond job to display.\n");
    }
}

// add jobs to the list, executed right before bg task starts
void addJob(char *name){
    for (int i=0; i<sizeof(jobs); i++){
        if (jobs[i][0] == '\0') {
            strcpy(jobs[i], name);
            break;
        }
    }
    jobCount ++ ;
}

// remove jobs from job list, executed when a bg task is done
void removeJob(char *name){
    for (int i=0; i<sizeof(jobs); i++){
        if (strcmp(jobs[i], name) == 0){ // if found job
            jobs[i][0] = '\n'; // clear the job
            jobCount -- ;
            break;
        }
    }
}


// wrapper to choose which task to execute depending on user input
void execute(char *originalInput, char *args[], int background){
    // if it's a background job, add it to the list
    if (background){
        addJob(originalInput);
    }
    if (strcmp(originalInput, "history") == 0){
        displayHistory();
    }
    else if (strcmp(originalInput, "cd") == 0){ // change directory
        chdir(args[1]);
    }
    else if (strcmp(originalInput, "pwd") == 0){ // present working directory
        printf("%s\n", getcwd(NULL, 1000));
    }
    else if (strcmp(originalInput, "exit") == 0){ // exit the program
        exit(0);
    }
    else if (strcmp(originalInput, "jobs") == 0){ // display background jobs
        displayJobs();
    }
    else {
        printf("args1: %s\n", args[0]);
        execvp(args[0], args);
    }
    // at this line, the background job is finished executing, so remove it from the list
    if (background){
        removeJob(originalInput);
    }
}




// read

/**
 * setup() reads in the next command line, separating it into distinct tokens
 * using whitespace as delimiters. setup() sets the args parameter as a
 * null-terminated string.
 */
int setup(char originalInput[], char inputBuffer[], char *args[],int *background, int *wantHistory, int fromStdin, int *builtIn) {
    int length, /* # of characters in the command line */
    i, /* loop index for accessing inputBuffer array */
    start, /* index where beginning of next command parameter is */
    ct; /* index of where to place the next parameter into args[] */

    ct = 0;

    /* read what the user enters on the command line */
    if (fromStdin) {
        length = read(STDIN_FILENO, inputBuffer, MAX_LINE);
        strcpy(originalInput, inputBuffer); // save original input to history later on
        originalInput[length-1] = '\0'; // remove \n from original input
    }
    else {
        length = strlen(inputBuffer) + 1;
        strcpy(originalInput, inputBuffer);
    }


    start = -1;
    if (length == 0)
        exit(0); /* ^d was entered, end of user command stream */
    if (length < 0){
        perror("error reading the command");
        exit(-1); /* terminate with error code of -1 */
    }
    /* examine every character in the inputBuffer */
    for (i=0;i<length;i++) {
        switch (inputBuffer[i]){
            case ' ':
            case '\t' : /* argument separators */
                if(start != -1){
                    args[ct] = &inputBuffer[start]; /* set up pointer */
                    ct++;
                }
                inputBuffer[i] = '\0'; /* add a null char; make a C string */
                start = -1;
                break;
            case '\n': /* should be the final char examined */
                if (start != -1){
                    args[ct] = &inputBuffer[start];
                    ct++;
                }
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
                break;
            default : /* some other character */
                if (start == -1)
                    start = i;
                if (inputBuffer[i] == '&'){
                    *background = 1;
                    inputBuffer[i] = '\0';
                }
        }
    }
    args[ct] = NULL; /* just in case the input line was > 80 */


    // check if history command is valid
    int validInput = 1;
    if (isHistoryCommand(originalInput)){
        *wantHistory = 1;
        if (isValidHistory(originalInput) == 0) validInput = 0;
        else pushToHistory(getHistory(originalInput));
    }
    else {
        pushToHistory(originalInput);
    }

    // flag build-in command
    if (strcmp(args[0], "cd") == 0      ||
        strcmp(args[0], "pwd") == 0     ||
        strcmp(args[0], "exit") == 0    ||
        strcmp(args[0], "jobs") == 0)   {
            *builtIn = 1;
    }

    return validInput;

}



int main(void) {

    char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
    int background; /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE/+1]; /* command line (of 80) has max of 40 arguments */

    int wantHistory;
    int fromStdin;
    int builtIn;

    while (1){ /* Program terminates normally inside setup */
        background = 0;
        wantHistory = 0;
        fromStdin = 1;
        builtIn = 0;
        char originalInput[MAX_LINE];

        printf(" COMMAND->\n");
        int validInput = setup(originalInput, inputBuffer, args, &background, &wantHistory, fromStdin, &builtIn); /* get next command */

        /* (1) fork a child process using fork() */
        int pid = fork();
        int status;

        /* (2) the child process will invoke execvp() */
        if (pid == 0) {
            if (builtIn == 0){ // child process will not execute built-in functions
                if (wantHistory){
                    if (validInput){ // valid history call
                        printf("%s\n", getHistory(originalInput)); // write original call on console
                        setup(originalInput, getHistory(originalInput), args, &background, &wantHistory, 0, 0);
                        execute(originalInput, args, background);
                    }
                    else {
                        printf("Invalid history call.\n");
                    }
                }
                else {
                    execute(originalInput, args, background);
                }
            }

            /* Status of the child. If this line runs, it means execvp failed. */
            // exit(0);
        }

        /* (3) if background == 0, the parent will wait,
         otherwise returns to the setup() function. */
        else {
            if (background == 0){
                if (builtIn){
                    execute(originalInput, args, background);
                }
                waitpid(pid, &status, WUNTRACED | WCONTINUED);
            }
            else {
                /* Run concurrently */
            }


        }



    }
}
