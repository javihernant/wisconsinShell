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

void execute_cmd(char * cmd, char ** pathv, int * pathc, size_t * pathc_limit){

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
    int my_argv_size = 10;
    char ** my_argv = malloc(my_argv_size*sizeof(char *));

    //store each argument in my_argv array
    while(begin != NULL){
        strsep(&end," \n\t");

        if((*begin != '\0')&&(*begin != ' ')){

            if(my_argc>=my_argv_size){

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

    if(my_argc==0){
        return;
    }

    if (strcmp(my_argv[0], "exit") == 0){
        exit(0);
    }else if (strcmp(my_argv[0], "cd") == 0){
        printf("change directory\n");
        if(my_argc != 2){
            print_error();
            return;
        }
        if (chdir(my_argv[1])!=0){
            print_error();
            return;
        }

    
    }else if (strcmp(my_argv[0], "path") == 0){
        printf("change path\n");
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
            pathv[*pathc] = my_argv[i]; 
            *pathc+=1;
        }
    
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
                printf("%s\n",my_argv[i]);
            }   

            exit(0);
        }else{
            int rc_wait = wait(NULL);
            printf("I'm parent(pid:%d). Done waiting child (pid:%d). rc_wait:%d\n",(int) getpid(), rc, rc_wait);
        }

    }
    
    free(my_argv);

}

int main(int argc, char *argv[]) {
    assert(argc <= 2);
    int interactive_mode = argc==1 ? 1 : 0;
    if(interactive_mode){
        printf("wish> ");
    }

    int pathc = 1; //number of paths stored
    size_t pathc_limit = 10;
    char ** pathv = malloc(pathc_limit * sizeof(char*));
    pathv[0] = strdup("/bin");
    size_t n=0;
    char * cmd = NULL;
    
    while((n=getline(&cmd,&n,stdin))!= -1){
        
        execute_cmd(cmd,pathv,&pathc,&pathc_limit);
        
        if(interactive_mode){
            printf("wish> ");
        }
        
    }

    //free memory for first path, remaings are in cmd
    free(pathv[0]);
    
    //free array containing pointers to paths
    free(pathv);

    //free line of commands
    free(cmd);
    exit(0);
}