#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

/*
	Question: linecap: line capacity?
	
*/
int getcmd(char *prompt, char *args[], int *background){
	int length, i = 0;
	char *token, *loc;
	char *line = NULL;
	size_t linecap = 0;

	printf("%s", prompt);
	length = getline(&line, &linecap, stdin);

	if (length <= 0){
		exit(-1);
	}

	//check if background is specified
	if ((loc = index(line, '&'))!=NULL) {
		*background = 1;
		*loc = ' ';
	} else
		*background = 0;
	
	while ((token = strsep(&line, " \t\n")) != NULL){
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
	int bg;FGHJG
	while(1){
		bg = 0;
		int cnt =getcmd("\n>> ", args, &bg);

		printf("%d", cnt);
	}
	
}