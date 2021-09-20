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

void execute_cmd(char * cmd, char ** pathv, int * pathc, size_t * pathc_limit, char ** prev_right_piece){

    char * begin;
    char * end;
    // printf("%s",cmd);
    int pipe_active=0;
    char * right_piece=cmd; //chop the command in left side for the command and the right for the piping files. 
    strsep(&right_piece, ">");
    if(right_piece!=NULL){
        // printf("there is a right piece!\n");
        //check for multiple >
        char * pipe_error=right_piece;
        strsep(&pipe_error, ">");
        if(pipe_error != NULL){
            print_error();
            return;
        }
        //check for multiple files or no files at all. 
        begin=right_piece;
        end=right_piece;
        int filec=0;
        while(begin != NULL){
            strsep(&end," \n\t");

            if((*begin != '\0')&&(*begin != ' ')){
                right_piece = begin;
                filec+=1;
            }
            
            begin=end;

        }


        if(filec!=1){
            print_error();
            return;
        }
        

        pipe_active=1;

    }

    begin = cmd;
    end = cmd;
    int my_argc = 0;
    int my_argv_size = 10;
    char ** my_argv = malloc(my_argv_size*sizeof(char *));

    //store each argument in my_argv array
    while(begin != NULL){
        strsep(&end," \n\t");

        if((*begin != '\0')&&(*begin != ' ')){

            if(my_argc+1>=my_argv_size){

                my_argv_size *= 2;
                char ** tmp_argv = realloc(my_argv, my_argv_size*sizeof(char *));
                if (tmp_argv != NULL) {
                    my_argv = tmp_argv;
                } else {
                    fprintf(stderr, "realloc failed\n");
                    exit(1);
                }
                
            }
            my_argv[my_argc++] = begin;
        }
        
        begin=end;

    }
    my_argv[my_argc] = NULL;

    if(pipe_active && my_argc==0){
        print_error();
        return;
    }

    //if command line is empty, return and keep reading lines.
    if(my_argc==0){
        return;
    }

    if (strcmp(my_argv[0], "exit") == 0){
        if(my_argc > 1){
            print_error();
            return;
        }
        exit(0);
    }else if (strcmp(my_argv[0], "cd") == 0){
        // printf("change directory\n");
        if(my_argc != 2){
            print_error();
            return;
        }
        if (chdir(my_argv[1])!=0){
            print_error();
            return;
        }

    
    }else if (strcmp(my_argv[0], "path") == 0){
        // printf("change path\n");
        //free memory for each char * pointer in pathv
        for(int i=0; i<*pathc; i++){
            free(pathv[i]);
        }

        *pathc=0;
        int do_realloc = 0;
        //if paths limit has been exceeded, realloc a bigger path array
        while(my_argc-1 > *pathc_limit){
            do_realloc = 1;
            *pathc_limit*=2;
        }

        if(do_realloc){
            char ** tmp_pathv = realloc(pathv,*pathc_limit*sizeof(char*));
            if (tmp_pathv != NULL) {
                pathv = tmp_pathv;
            } else {
                fprintf(stderr, "realloc failed\n");
                exit(1);
            }
            
        }

        //store paths in path array
        for(int i=1; i<my_argc;i++){
            // printf("pathc:%d",*pathc);
            pathv[*pathc] = strdup(my_argv[i]); 
            *pathc+=1;
        }
    
    }else{
        //traverse path array to find one that contains given program

        int found_program = 0;
        char path2program[100];
        for(int i=0; i<*pathc; i++){
            strcpy(path2program,pathv[i]);

            if(pathv[i][strlen(pathv[i])-1] != '/'){
                strcat(path2program, "/");
            }
            strcat(path2program, my_argv[0]);
            // printf("looking at %s\n",path2program);

            if(access(path2program, X_OK)==0){
                found_program = 1;
                break;
            }
        }

        if(!found_program){
            print_error();
            return;
        }

        if(pipe_active){
            if(*prev_right_piece == NULL){
                    *prev_right_piece = strdup(right_piece);

            }else{
                if(strcmp(right_piece,*prev_right_piece)==0){
                    return;
                }
            }
        }
        
        
        int rc= fork();
        if(rc<0){
            fprintf(stderr, "fork failed\n");
            exit(1);

        }else if(rc==0){
            //child process runs external commands.    
            // printf("Hello, I'm child process:%d\n",(int) getpid());

            if(pipe_active){
                
                int fd;
                if((fd= open(right_piece, O_CREAT|O_TRUNC|O_RDWR,S_IRWXU))==-1){
                    printf("open:%s\n",strerror(errno));
                    printf("directory:%s\n",right_piece);
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

            execv(path2program, my_argv);

            // for(int i=0; i<my_argc; i++){
            //     printf("%s\n",my_argv[i]);
            // }   

            exit(0);
        }else{
            int rc_wait = wait(NULL);
            // printf("I'm parent(pid:%d). Done waiting child (pid:%d). rc_wait:%d\n",(int) getpid(), rc, rc_wait);
        }

    }
    
    free(my_argv);

}

int main(int argc, char *argv[]) {
    if(argc > 2){
        print_error();
        exit(1);
    }
    int interactive_mode = argc==1 ? 1 : 0;
    FILE *stream;
    if(interactive_mode){
        stream = stdin;
        printf("wish> ");
        
    }else{
        stream = fopen(argv[1], "r");
        if(stream == NULL){
            print_error();
            exit(1);
        }
    }

    int pathc = 1; //number of paths stored
    size_t pathc_limit = 10;
    char ** pathv = malloc(pathc_limit * sizeof(char*));
    pathv[0] = strdup("/bin");
    size_t n=0;
    char * cmd = NULL;
    
    char * prev_right_piece = NULL;
    while((n=getline(&cmd,&n,stream))!= -1){
        
        char * begin=cmd;
        char * end=cmd;
        while(begin != NULL){
            strsep(&end,"&");
            execute_cmd(begin,pathv,&pathc,&pathc_limit, &prev_right_piece);
            begin = end;
        }
        
        
        if(interactive_mode){
            printf("wish> ");
        }
        
    }
    
    free(prev_right_piece);
    //free array containing pointers to paths
    free(pathv);

    //free line of commands
    free(cmd);
    exit(0);
}