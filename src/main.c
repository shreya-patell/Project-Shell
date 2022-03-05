#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>




//required global variables


static char *args[512];
static char userPrompt[512];
pid_t pid;

int fd;
int flagged, length;
int backgroundFlagged;
int numLines;
int envFlag;
int flagged_without_pipe,  outputDir, inpDir;
int status;

char *history_file;
char *buffer;
char *cmdExec[512];
char *inpDirFD;
char *outDirFD;
char curDir[1024];



static int builtInCMD(char *, int, int, int);
static int execCommand(int, int, int, char *);
char *ignoreWhiteSpace (char* );
char *ignoreDoubleQuote(char* );
void zeroVariable(); 
void printHist ();
void environmment();
void set_environment_variables();
void chgDir();
void tokenizeSpace (char *);
void redirInpOut(char *);
void redirectInp(char *);
void redirOutput(char *);
void checkForPipe ();
void promptUser(); 
void signalHandler(int );
void checkBackground();


//function to handle signals 
void signalHandler(int sig_num) {

    signal(SIGINT, signalHandler);
    fflush(stdout);
    return;
}

//function used to zero certain variables that are actively used
void zeroVariable() {

	fd = 0;
	flagged = 0;
	length = 0;
	numLines = 0;
	flagged_without_pipe = 0;
	outputDir = 0;
	inpDir = 0;
	curDir[0] = '\0';
	userPrompt[0] = '\0';
 	pid = 0;
	envFlag = 0;
}

//used to print the history of entered commands
void printHist () {
  	
  	register HIST_ENTRY **list;
    register int i;

    list = history_list ();
    if (list)
    	for (i = 0; list[i]; i++)
            printf ("%d: %s\n", i + history_base, list[i]->line);
    return;
}

//creates first line that user is presented with current directory
void promptUser() {

	if (getcwd(curDir, sizeof(curDir)) != NULL) {

		strcat(userPrompt, curDir);
		strcat(userPrompt, "$ ");
	}
	else {

		perror("Error in getting curent working directory: ");
	}
	return;
}

//removes whitespace from input
char *ignoreWhiteSpace (char* str) {

	int i = 0, j = 0;
	char *temp;
	if (NULL == (temp = (char *) malloc(sizeof(str)*sizeof(char)))) {
		perror("Memory Error: ");
		return NULL;
	}

	while(str[i++]) {

		if (str[i-1] != ' ')
			temp[j++] = str[i-1];
	}
	temp[j] = '\0';
	return temp;
}

//removes doubles quotes from input
char *ignoreDoubleQuote (char *str) {

	int i = 0, j = 0;
	char *temp;
	if (NULL == (temp = (char *) malloc(sizeof(str)*sizeof(char)))) {
		perror("Memory Error: ");
		return NULL;
	}

	while(str[i++]) {

		if (str[i-1] != '"'){

			temp[j++] = str[i-1];
		}
	}
	temp[j] = '\0';
	return temp;
}

//changes the directory location
void chgDir() {

	char *dir = "/home";

	if ((args[1]==NULL) || (!(strcmp(args[1], "~") && strcmp(args[1], "~/"))))
		chdir(dir);
	else if (chdir(args[1]) < 0)
		perror("No such file or directory: ");

}

//handles basic functionality that doesn't require much complexity
static int builtInCMD (char *cmdExec, int input, int isfirst, int islast) {

	char *newCmdExec;

	newCmdExec = strdup(cmdExec);

	tokenizeSpace (cmdExec);
	checkBackground ();

	if (args[0] != NULL) {
		if (!(strcmp(args[0], "exit") && strcmp(args[0], "quit")))
			exit(0);

		if (strcmp(args[0], "echo")) {

			cmdExec = ignoreDoubleQuote(newCmdExec);
			tokenizeSpace(cmdExec);
			//echoCall(cmdExec);
		}

		if (!strcmp("cd", args[0])) {

			chgDir();
			return 1;
		}

		if (!strcmp(args[0], "history")) {
			
			printHist();
			return 1;
		}
		if(strcmp(buffer,"help") == 0){
			int c;
			FILE *file;
			file = fopen("readme.txt","r");
			if(file){
				while((c=getc(file)) != EOF)
					putchar(c);
			fclose(file);
			}
			return 1;
		}

	}
	return (execCommand(input, isfirst, islast, newCmdExec));
}

//function for I/O redirection (redirects output and input)
void redirInpOut (char *cmdExec) {

	char *val[128];
	char *newCmdExec, *x1, *x2;
	newCmdExec = strdup(cmdExec);

	int m = 1;
	val[0] = strtok(newCmdExec, "<");
	while ((val[m] = strtok(NULL,">")) != NULL) m++;

	x1 = strdup(val[1]);
	x2 = strdup(val[2]);

	inpDirFD = ignoreWhiteSpace(x1);
	outDirFD = ignoreWhiteSpace(x2);

	tokenizeSpace(val[0]);
	return;
}

//function for I/O redirection (redirects input)
void redirectInp (char *cmdExec) {

	char *val[128];
	char *newCmdExec, *x1;
	newCmdExec = strdup(cmdExec);

	int m = 1;
	val[0] = strtok(newCmdExec, "<");
	while ((val[m] = strtok(NULL,"<")) != NULL) m++;

	x1 = strdup(val[1]);
	inpDirFD = ignoreWhiteSpace(x1);

	tokenizeSpace (val[0]);
	return;
}

//function for I/O redirection (redirects output)
void redirOutput (char *cmdExec) {

	char *val[128];
	char *newCmdExec, *x1;
	newCmdExec = strdup(cmdExec);

	int m = 1;
	val[0] = strtok(newCmdExec, ">");
	while ((val[m] = strtok(NULL,">")) != NULL) m++;

	x1 = strdup(val[1]);
	outDirFD = ignoreWhiteSpace(x1);

	tokenizeSpace (val[0]);
	return;
}

//echo function
void echoCall(char *cmdExec){
	
	char echoText[1000];
	strncpy(echoText,cmdExec+5,sizeof(cmdExec));
	printf("%s\n",echoText);


	return;

}


//command handler, this function handles input and determines what should be done
static int execCommand (int input, int first, int last, char *cmdExec) {

	int mypipefd[2], ret, inputFD, output_fd;

	if (-1 == (ret = pipe(mypipefd))) {
		perror("pipe error: ");
		return 1;
	}

	pid = fork();

	if (pid == 0) {
		if (first == 1 && last == 0 && input == 0) {
			dup2 (mypipefd[1], 1);
		}
		else if (first == 0 && last == 0 && input != 0) {
			dup2 (input, 0);
			dup2 (mypipefd[1], 1);
		}
		else {
			dup2 (input, 0);
		}

		if (strchr(cmdExec, '<') && strchr(cmdExec, '>')) {
			inpDir = 1;
			outputDir = 1;
			redirInpOut (cmdExec);
		}
		else if (strchr(cmdExec, '<')) {
			inpDir = 1;
			redirectInp (cmdExec);
		}
		else if (strchr(cmdExec, '>')) {
			outputDir = 1;
			redirOutput (cmdExec);
		}

		if (outputDir) {
			if ((output_fd = creat(outDirFD, 0644)) < 0) {
				fprintf(stderr, "Cannot open: %s\n", outDirFD);
				return (EXIT_FAILURE);
			}
			dup2 (output_fd, 1);
			close (output_fd);
			outputDir = 0;
		}

		if (inpDir) {
			if ((inputFD = open(inpDirFD, O_RDONLY, 0)) < 0) {
				fprintf(stderr, "Failed to open %s\n", inpDirFD);
				return (EXIT_FAILURE);
			}
			dup2 (inputFD, 0);
			close (inputFD);
			inpDir = 0;
		}
		checkBackground();
		if (!strcmp (args[0], "echo")) {
			echoCall(cmdExec);
		}
		else if (execvp(args[0], args) < 0) {
			fprintf(stderr, "%s: command cannot be found!\n",args[0]);
		}
		exit(0);
	}

	else {

		if (backgroundFlagged == 0)
			waitpid(pid,0,0);
	}

	if (last == 1)
		close(mypipefd[0]);

	if (input != 0)
		close(input);

	close(mypipefd[1]);
	return (mypipefd[0]);
}

//tokenizes spaces within an input
void tokenizeSpace (char *str) {

	int m = 1;

	args[0] = strtok(str, " ");
	while ((args[m] = strtok(NULL," ")) != NULL) m++;
	args[m] = NULL;
}

//checks to see if anything is running in background currently and sets backgroundFlagged variable accordingly
void checkBackground () {

	int i = 0;
	backgroundFlagged = 0;
	
	while (args[i] != NULL) {
		if (!strcmp(args[i], "&")) {
			backgroundFlagged = 1;
			args[i] = NULL;
			break;
		}
		i++;
	}

}


//checks the input for piping characters
void checkForPipe () {

	int i, n = 1, input = 0, first = 1;

	cmdExec[0] = strtok(buffer, "|");
	while ((cmdExec[n] = strtok(NULL, "|")) != NULL) n++;

	cmdExec[n] = NULL;
	
	for (i = 0; i < n-1; i++) {

		input = builtInCMD(cmdExec[i], input, first, 0);	
		first = 0;
	} 

	input = builtInCMD(cmdExec[i], input, first, 1);
	return;
}


int main() {

	int status;
	system ("clear");
	signal(SIGINT, signalHandler);
	char newLineChar = 0;
	using_history();

	do {

		zeroVariable();
		promptUser();
		buffer = readline (userPrompt);


		if(strcmp(buffer,"\n"))
			add_history (buffer);

		if (!(strcmp(buffer, "\n") && strcmp(buffer,"")))
			continue;

		if (!(strncmp(buffer, "exit", 4) && strncmp(buffer, "quit", 4))) {

			flagged = 1;
			break;
		}



		checkForPipe();

		if (backgroundFlagged == 0)
			waitpid(pid,&status,0);
		else
			status = 0;

	} while(!WIFEXITED(status) || !WIFSIGNALED(status));

	if (flagged == 1) {

		printf("\nClosing shell!!\n");
		exit(0);
	}

	return 0;
}