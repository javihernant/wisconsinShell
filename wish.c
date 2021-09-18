#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>



       
void print_error(){
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message)); 
}

void execute_cmd(char * cmd, size_t size){

    // printf("%s",cmd);
    int pipe_active=0;
    char * right_piece=cmd; //chop the command in left side for the command and the right for the piping files. 
    strsep(&right_piece, ">");
    if(right_piece!=NULL){
        printf("there is a right piece!\n");
        //check for multiple >
        char * pipe_error=right_piece;
        strsep(&pipe_error, ">");
        if(pipe_error != NULL){
            print_error();
            return;
        }
        //check for multiple files. Assume no spaces after '>'. i.e (> file1) not allowed. 
        pipe_error=right_piece;
        int passed_space = 0;
        for(; *pipe_error!='\0'; pipe_error++){
            if(!passed_space && (*pipe_error == ' ' || *pipe_error == '\t' || *pipe_error == '\n')){
                passed_space = 1;
            }

            if(passed_space && (*pipe_error != ' ' && *pipe_error != '\t' && *pipe_error != '\n')){
                print_error();
                return;
            }
        }

        pipe_active=1;

    }

    char * begin = cmd;
    char * end = cmd;
    int my_argc = 0;
    int my_args_size = 10;
    char ** my_args = malloc(my_args_size*sizeof(char *));

    //store each argument in my_args array
    while(begin != NULL){
        strsep(&end," \n\t");

        if((*begin != '\0')&&(*begin != ' ')){

            if(my_argc>=my_args_size){

                my_args_size *= 2;
                my_args = realloc(my_args, my_args_size*sizeof(char *));
                
                if(my_args == NULL){
                    printf("realloc failed");
                }
            }
            my_args[my_argc++] = begin;
        }
        
        begin=end;

    }

    if(my_argc==0){
        return;
    }

    if (strcmp(my_args[0], "exit") == 0){
        exit(0);
    }else if (strcmp(my_args[0], "cd") == 0){
        printf("change directory\n");
    
    }else if (strcmp(my_args[0], "path") == 0){
        printf("change path\n");
    
    }else{
        int rc= fork();
        if(rc<0){
            fprintf(stderr, "fork failed\n");
            exit(1);

        }else if(rc==0){
            //child process runs non-built-in commands.    
            printf("Hello, I'm child process:%d\n",(int) getpid());
            if(pipe_active){
                int fd;
                if((fd= open(right_piece, O_CREAT|O_TRUNC|O_RDWR,S_IRWXU))==-1){
                    printf("open:%s",strerror(errno));
                    exit(1);
                }

                int dup_err;
                if((dup_err=dup2(fd, STDOUT_FILENO))==-1){
                    printf("dup2:%s",strerror(errno));
                    exit(1);
                };

                if((dup_err=dup2(fd, STDERR_FILENO))==-1){
                    printf("dup2:%s",strerror(errno));
                    exit(1);
                };

                if(close(fd)==-1){
                    printf("close:%s",strerror(errno));
                    exit(1);
                }
                
            }

            for(int i=0; i<my_argc; i++){
                printf("%s\n",my_args[i]);
            }   

            exit(0);
        }else{
            int rc_wait = wait(NULL);
            printf("I'm parent(pid:%d). Done waiting child (pid:%d). rc_wait:%d\n",(int) getpid(), rc, rc_wait);
        }

    }


}

int main(int argc, char *argv[]) {
    assert(argc <= 2);
    int interactive_mode = argc==1 ? 1 : 0;
    if(interactive_mode){
        printf("wish> ");
    }

    size_t n=0;
    char * cmd = NULL;
    
    while((n=getline(&cmd,&n,stdin))!= -1){
        
        execute_cmd(cmd,n);
        
        if(interactive_mode){
            printf("wish> ");
        }
        
    }
    
    free(cmd);
    exit(0);
}