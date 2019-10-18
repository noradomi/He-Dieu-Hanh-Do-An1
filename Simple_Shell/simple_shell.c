
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "history.h"
#define FALSE 0
#define TRUE 1
#define MAX_LINE 80     //Chiều dài tối đa có của lệnh
//Khai báo biến lưu trữ các thành phần trong câu lệnh, mỗi cmd[i] sẽ lưu trữ một thành phần của lệnh cách nhau bởi khoảng trắng(nếu có)
char *Cmd[80];
//Khai báo các biến đánh dấu
int FirstCmd = TRUE; // Đánh dấu bắt đầu lệnh
int LastCmd = FALSE;//Đánh dấu kết thúc lệnh
int IsRedirect = FALSE;//Đánh dấu để kiểm tra có phải 
int IsExit = FALSE;//Đánh dấu kiểm tra có thoát khỏi chương trình hay không
int IsRead=FALSE;//Đánh dấu kiểm tra xem có phải đọc file hay không,nếu là FALSE tức là ở chế độ ghi file
//int check=TRUE;

int IsThread=FALSE;//Kiếm tra xem lệnh có yêu cầu chạy background hay không
char Path_File[1024];//Lưu tên file để đọc,ghi(nếu có)



void CdPath(char *cmd[], int ele_cmd) {
    char *path = cmd[1];
    char *home = (char *) (malloc(MAX_LINE * sizeof(char)));
    strcpy(home, getenv("HOME"));//Tìm kiếm địa chỉ chuỗi môi trường rồi lưu vào biến home
    if (ele_cmd == 1) {
        chdir(home); //Thay đổi địa chỉ đường dẫn hiện tại đến địa chỉ home
        return;
    }
    if (cmd[1][0] == '~') {
        if ((strlen(cmd[1]) == 1) || (strlen(cmd[1]) == 2 && cmd[1][1] == '/')) {
            chdir(home);//Thay đổi địa chỉ đường dẫn có dạng như (~link hoặc /link)
            return;
        }
        if (strlen(cmd[1]) > 2 && cmd[1][1] == '/') {
            path = strcat(strcat(home, "/"), cmd[1] + 2);
        }
    }
    if (chdir(path) == -1) {
        printf("%s : No such file or directory.\n", cmd[1]);//Trường hợp di chuyển đường dẫn không thành công thì thông báo ra
    }
    return;
}


//Hàm thực thi lệnh
int Execute(int data){
	
	pid_t pid;
	int fd[2];
	int status = 1;
	
	status = pipe(fd);//Khởi tạo luồng
	
	if(status==-1)
	{ perror("Load pipe error\n"); //Tạo luồng thất bại
	return -1;
	}
	
	pid = fork();//Tạo một process mới(process con)
	
	//Nếu trả về -1(thất bại)
	if (pid==-1){ perror("Fork error\n"); return -1;}

	//Nếu trả về 0(thành công), thực thi process con
	if (pid==0){

		//Chạy pipe có biến output file_fd đóng vai trò là kết quả output của pipe   
		if(IsRedirect && IsRead==FALSE){
			FILE *fp = fopen(Path_File, "w");
			if(fp==NULL){
				printf("Cannot open file %s\n", Path_File);
				return -1;
			}
			int file_fd = fileno(fp);

			dup2(data, STDIN_FILENO);
			dup2(file_fd, STDOUT_FILENO);
		}
		else if(IsRedirect && IsRead==TRUE)
			{FILE *fp = fopen(Path_File, "r");
			if(fp==NULL){
				printf("Cannot open file %s\n", Path_File);
				return -1;
			}
			int file_fd = fileno(fp);

			dup2(data, STDOUT_FILENO);
			dup2(file_fd, STDIN_FILENO);
			
			}
		//Nếu chỉ có một lệnh tức là không có input cho luồng kế tiếp,chỉ có output kết quả ra
		else if(FirstCmd && !LastCmd){
			
			dup2(fd[1], STDOUT_FILENO);
			
		}
		//Nếu có luồng ,tức là output của luồng đầu là input cho luồng sau
		else if (!LastCmd && !FirstCmd){
			dup2(data, STDIN_FILENO);
			dup2(fd[1], STDOUT_FILENO);
		}
		//Trường hợp kết thúc luồng,chỉ có input luồng trước truyền vào truyền vào
		else if (!FirstCmd && LastCmd){
			dup2(data, STDIN_FILENO);
		}
		//Xây dựng cd command,cho phép thay đổi địa chỉ hiện tại
		if(strcmp(Cmd[0], "cd") == 0){
			int len_cmd=0;
			while(Cmd[len_cmd]!=NULL)
			{
				len_cmd++;
			}
			CdPath(Cmd,len_cmd);
		}
		else if(strcmp(Cmd[0],"history")==0)     // Gõ lệnh history để xem lịch sử các lệnh
		{
			get_history();
			exit(1);
		}
		else if(execvp(Cmd[0], Cmd) == -1)  //Nếu thực thi lệnh thất bại,thì thông báo cho user
		{    
			printf("Command \"%s\" cannot be executed ", Cmd[0]);   
			exit(1);
		}
		
		close(fd[0]);
	}
	//Chạy luồng cha
	else if(!IsThread){
		//Chờ luồng con kết thúc
		//int status1;
		waitpid(pid, NULL,0);
		//wait(&status);
		//printf("%d",pid);
		if(IsRedirect){
			close(fd[1]);
			return 0;
		}
       if(LastCmd){
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
	
	
	
	return fd[0];
}

//Hàm reset lại các global variables trở lại giá trị mặc định ban đầu
int ClearVariables(){
	int i;
	LastCmd = FALSE;
	FirstCmd = TRUE;
	//Clear các lệnh cmd
	for(i=0; i<80; i++) 
		Cmd[i] = '\0';
	//Xóa Path_File
	Path_File[0] = '\0';
	IsRedirect = FALSE;
	IsRead=FALSE;//Reset đánh dấu đọc file
	IsThread=FALSE;//Reset kiểm tra luồng

	return 0;
}

//Tách các thành phần trong lệnh mà user nhập vào và gọi lệnh thực thi sau khi tách
int ParseUserCommand(char *input){

	//Kiểm tra lệnh nhập vào là exit thì thoát chương trình
	if(strcmp(input, "exit") == 0 || strcmp(input, "quit") == 0){
		IsExit = TRUE;
		exit(0);
	}
	char *next = input;
	
	
	int len = 0;//Chiều dài 1 block của lệnh
	
	int n=0;//Số lượng thành phần có trong user command 
	   
	int LastFd;
	

	//Phân tích command và gọi hàm Execute()
	while(*next){
		//Bỏ qua các kí tự space
		while(*next == ' ') 
		{next++;}
		input = next;
		
		while((*next != ' ' && *next != '\0')){
			//Tìm kiếm các thành phần trong lệnh
				if(*next =='|' || *next == '>' || *next== '<'){
					printf("\n");
					if(*next == '>'){  
						IsRedirect = TRUE; //Có thực thi redirect
						++next;
						
						while(*next == ' ') next++;//Bỏ qua space
						strcpy(Path_File, next);//Lưu tên file để chạy redirection
					}
					if(*next == '<'){
						IsRedirect = TRUE; //Có thực thi redirect
						IsRead=TRUE;
						++next;
						
						while(*next == ' ') next++;//Có thực thi redirect
						strcpy(Path_File, next);//Lưu tên file để chạy redirection
					}

					
					LastFd= Execute(LastFd);//Lấy file decriptor của command cuối

					
					if(FirstCmd) 
					FirstCmd = FALSE;//Gán FirstCmd thành FALSE sau khi thực thi xong lệnh đầu

					memset(Cmd, 0, sizeof(Cmd));
					n = 0;
					break;
				
			}
			
			len++; next++;
		}
	
		if(len!=0){
			//Khởi gán gán từng phần tử trong user command vào cmd[i]
			Cmd[n] = (char *)malloc(len+1);
			strncpy(Cmd[n], input, len);
			Cmd[n][len] = '\0';
			
			n++;
		}
		
		input = ++next;
		len = 0;
	}

	//Thực thi last command
	LastCmd = TRUE;
	//Nếu không yêu cầu redirection thì thực thi các lệnh mà không cần PathFile
	if(!IsRedirect){
		LastFd = Execute(LastFd);
	}
	
	
	
	ClearVariables();//Reset lại các biến môi trường
	
	return 0;
}

int main(){
	
    int should_run = 1;
	while(should_run){
		char *input = (char *) (malloc(MAX_LINE * sizeof(char)));
		printf("osh> ");
       		 fflush(stdout);
        	fgets(input, MAX_LINE, stdin);//Nhập vào lệnh
        	input[strlen(input) - 1] = '\0';
		if(strcmp(input,"")!=0) //Nếu lệnh khác rỗng thì lưu vào history
		{set_history(input);}
		
		if(strcmp(input,"!!")!=0)   //Nếu lệnh khác !! thì lưu lại để khi gọi !! sẽ thực thi câu lệnh trước đó
		{
			set(input);
		
		}
		if(strcmp(input,"!!")==0)  //Lấy lệnh trước đó thực thi lại khi gõ !!
		{
			if(get()!=NULL)
				input=get();	
			else
				{
				printf("No commands in history\n");   //Nếu không tồn tại lệnh trong history thì thông báo
				continue;
				}
		
			
		}
		
		if(strchr(input,'&')!=NULL)  //Kiểm tra xem có chạy background hay không
			{
			char*input1=input;
			IsThread=TRUE;
			int length=0;
			while(*input1!='\0')
				{length++;
				input1++;}
			
			char *temp=(char*)malloc(length);
			strncpy(temp,input,length-1);
			temp[length-1]='\0';
			input=temp;
		}
		
		
		ParseUserCommand(input);  // Gọi hàm phân tách các thành phần có trong command và thực thi
		
		
		if(IsExit)   
		{
		set(NULL);
		break;
		}
	
	
	}
	exit(0);
	return 0;
}

