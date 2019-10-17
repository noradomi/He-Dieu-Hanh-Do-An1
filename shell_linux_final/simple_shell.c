
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "history.h"
#define FALSE 0
#define TRUE 1
#define MAX_LINE 80
//Variable to store the commands
char *CMD[1024];
//Flags
int FIRST = TRUE;
int LAST = FALSE, REDIRECT = FALSE, EXIT = FALSE,ISREAD=FALSE;
int check=TRUE;
//Variable to store the file to redirect to
char PATH_FILE[1024];
int isequal=FALSE;
//Display prompt for use input
/*char *prompt(){
	char *cmd = NULL;
	//size_t size;
	printf("osh> ");
	if (getline(&cmd, &size, stdin) != -1){
		//remove newline
		cmd[strcspn(cmd, "\n")] = 0;
		return cmd;
	}
	perror("[*] Program terminating\n");
	return NULL;
}*/
void cd_path(char *cmd[], int ele_cmd) {
    char *path = cmd[1];
    char *home = (char *) (malloc(MAX_LINE * sizeof(char)));
    strcpy(home, getenv("HOME"));//tim kiem chuoi moi truong
    if (ele_cmd == 1) {
        chdir(home); //Tra ve chuoi moi truong neu chieu dai chi co mot(~)
        return;
    }
    if (cmd[1][0] == '~') {
        if ((strlen(cmd[1]) == 1) || (strlen(cmd[1]) == 2 && cmd[1][1] == '/')) {
            chdir(home);//tra den duong dan co dang (~link hoac /link)
            return;
        }
        if (strlen(cmd[1]) > 2 && cmd[1][1] == '/') {
            path = strcat(strcat(home, "/"), cmd[1] + 2);
        }
    }
    if (chdir(path) == -1) {
        printf("%s : No such file or directory.\n", cmd[1]);
    }
    return;
}


//Executes the current command
int execute(int in){
	//Declare variables
// int pid;
	pid_t pid;
	int fd[2];
	int status = 1;

	status = pipe(fd);
	//Failed pipe case
	if(status==-1){ perror("Pipe error\n"); return -1;}
	
	pid = fork();
	
	//Failed fork case
	if (pid==-1){ perror("Fork error\n"); return -1;}

	//Child process executes the command
	if (pid==0){

		//Redirection case has file_fd as output pipe
		if(REDIRECT && ISREAD==FALSE){
			FILE *fp = fopen(PATH_FILE, "w");
			if(fp==NULL){
				printf("Cannot open file %s\n", PATH_FILE);
				return -1;
			}
			int file_fd = fileno(fp);

			dup2(in, STDIN_FILENO);
			dup2(file_fd, STDOUT_FILENO);
		}
		else if(REDIRECT && ISREAD==TRUE)
			{FILE *fp = fopen(PATH_FILE, "r");
			if(fp==NULL){
				printf("Cannot open file %s\n", PATH_FILE);
				return -1;
			}
			int file_fd = fileno(fp);

			dup2(in, STDOUT_FILENO);
			dup2(file_fd, STDIN_FILENO);
			
			}
		//If it's first command, there's no input pipe
		else if(FIRST && !LAST){
			
			dup2(fd[1], STDOUT_FILENO);
			
		}
		//If it's neither first nor last, it has input and output pipe
		else if (!LAST && !FIRST){
			dup2(in, STDIN_FILENO);
			dup2(fd[1], STDOUT_FILENO);
		}
		//If it's the last, there's only input pipe
		else if (!FIRST && LAST){
			dup2(in, STDIN_FILENO);
		}
		//Builtin cd command
		if(strcmp(CMD[0], "cd") == 0){
			//chdir(CMD[1]);
			int len_cmd=0;
			while(CMD[len_cmd]!=NULL)
			{
				len_cmd++;
			}
			cd_path(CMD,len_cmd);
		}
		else if(strcmp(CMD[0],"history")==0)
		{
			get_history();
			exit(1);
		}
		else if(execvp(CMD[0], CMD) == -1){
			printf("Command \"%s\" cannot be executed ", CMD[0]);
			exit(1);
		}
		
		close(fd[0]);
	}
	//Parent
	else if(!isequal){
		//Wait for children to finish executing
		//int status1;
		waitpid(pid, NULL,0);
		
		//wait(&status);
		
		//printf("%d",pid);
		

		/*pid_t pid=fork();
		if(isequal){
		//printf("Cho");
		waitpid(pid, NULL, 0);
		exit(1);
		}*/
		if(REDIRECT){
			close(fd[1]);
			return 0;
		}

		if(LAST){
			FILE *input = fdopen(fd[0], "r");
			char buf[1024];
			//close(in);
			close(fd[1]);
			while(1){
				fgets(buf, 1024, input);
				if (feof(input)) break;
				printf("%s", buf);
			}
			fclose(input);
			printf("\n");
			return 0;
		}
		close(fd[1]);
	
	}
	
	//printf("FD 0 LA %d",fd[0]);
	
	return fd[0];
}

//Reset all global variables
int reset(){
	//Rest first and last flag
	int i;
	LAST = FALSE;
	FIRST = TRUE;
	//Clear the CMD
	for(i=0; i<1024; i++) CMD[i] = '\0';
	//Delete the PATH_FILE
	PATH_FILE[0] = '\0';
	REDIRECT = FALSE;//reset flag redirect
	ISREAD=FALSE;//reset flag read
	isequal=FALSE;

	return 0;
}

//Parse user input to an array of commands
int Parse(char *input){
	

	
	//Check if it's our shell's built-in command to exit
	if(strcmp(input, "exit") == 0 || strcmp(input, "quit") == 0){
		EXIT = TRUE;
		//set(NULL);
		exit(0);
	}
	
	//Declare variables
	char *next = input;
	int IN_QUOTES = FALSE;
	//Length of one block in a command
	int len = 0;
	//Number of arguments in a command
	int n=0;
	//Keep track of the last pipe
	int last_cmd_fd;
	

	//Parse the commands and call execute()
	while(*next){
		//Skip beginning white spaces
		while(*next == ' ') 
		{next++;}
		input = next;
		//Read in the next chunk
		while(IN_QUOTES || (*next != ' ' && *next != '\0')){
			//Case quotes
			if(*next == '\"' || *next == '\''){
				//Two cuts work here
				//if(!IN_QUOTES) *next = ' ';
				//len--;
				IN_QUOTES = !IN_QUOTES;
				//Original code
				if(!IN_QUOTES) input++;
				len--;
			}
			//Case not in quotes
			if(!IN_QUOTES) {
				//Execute the current command and go to next command
				if(*next =='|' || *next == '>' || *next== '<'){
					printf("\n");
					if(*next == '>'){
						REDIRECT = TRUE;
						++next;
						//Skip all whitespaces
						while(*next == ' ') next++;
						strcpy(PATH_FILE, next);
					}
					if(*next == '<'){
						REDIRECT = TRUE;
						ISREAD=TRUE;
						++next;
						//Skip all whitespaces
						while(*next == ' ') next++;
						strcpy(PATH_FILE, next);
					}

					//Gets the fd of the last command executed
					last_cmd_fd = execute(last_cmd_fd);

					//Set FIRST to FALSE after first command
					if(FIRST) FIRST = FALSE;

					memset(CMD, 0, sizeof(CMD));
					n = 0;
					break;
				}
			}
			//printf("%s",next);
			len++; next++;
		}
		//Copy the commands out and print it
		if(len!=0){
			//Set the arguments in the global variable
			CMD[n] = (char *)malloc(len+1);
			strncpy(CMD[n], input, len);
			CMD[n][len] = '\0';
			//Advance to the next argument
			n++;
		}
		//Proceed to the next chunk
		input = ++next;
		len = 0;
	}

	//Execute last command
	LAST = TRUE;
	//Don't need to execute if it's redirection
	if(!REDIRECT){
		last_cmd_fd = execute(last_cmd_fd);
	}
	
	/* if (strcmp(CMD[n-1], "&") == 0)
        {
            isequal =TRUE; 
            CMD[n- 1] = NULL;
            n--;
        }*/
	//Reset all global variables
	
	reset();
	
	return 0;
}
//Runs the shell
int main(){
	//char *input[MAX_LINE / 2 + 1];
    	int should_run = 1;
	while(should_run){
		
		
		char *input = (char *) (malloc(MAX_LINE * sizeof(char)));
		printf("osh> ");
       		 fflush(stdout);
        	fgets(input, MAX_LINE, stdin);
        	input[strlen(input) - 1] = '\0';
		if(strcmp(input,"")!=0)
		{set_history(input);}
		
		if(strcmp(input,"!!")!=0)
		{
			set(input);
			//printf("%s da set",get());
		}
		if(strcmp(input,"!!")==0)
		{
			if(get()!=NULL)
				input=get();	
			else
				{
				printf("No commands in history\n");
				continue;
				}
			//printf("%s",input);
			
		}
		//Check if received a command
		if(input==NULL){
			perror("[*] No command specified, exiting...\n");
			exit(1);
		}
		if(strchr(input,'&')!=NULL)
			{
			char*input1=input;
			isequal=TRUE;
			int length=0;
			while(*input1!='\0')
				{length++;
				input1++;}
			//printf("%d",length);
			char *temp=(char*)malloc(length);
			strncpy(temp,input,length-1);
			temp[length-1]='\0';
			input=temp;
		}
		//printf("%d %s",isequal,input);
		
		Parse(input);
		
		
		if(EXIT) 
		{
		set(NULL);
		break;
		}
	
	
	}
	exit(0);
	return 0;
}
