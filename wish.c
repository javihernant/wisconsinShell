#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

       
void print_error(){
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message)); 
}

int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);
    if(argc>2){
        print_error();
        exit(1);
    }

    int fd;
    int f_rc = 1;
    int int_mode = 1;

    if(argc==2){
        fd = open(argv[1], O_RDONLY); 
        dup2(fd, STDIN_FILENO);
        int_mode = 0;
    }

    char** my_paths = malloc(100* sizeof(char *));

    
    my_paths[0] = strdup("/bin");
    int c_paths = 1; 

   
    
    while(1){
        // dup2(stdout_cpy, STDERR_FILENO);
        // dup2(stderr_cpy, STDERR_FILENO);
        
        int c_chd_ps = 0;
        char * lineptr = NULL;
        size_t n;
        int err;
        if(int_mode){
            printf("wish> ");
        }
        
        err = getline(&lineptr, &n, stdin);
        if (err == -1){
            //all lines were already read
            exit(0);
        }
        
        
        
        int rc;

        char * cmd_lines[10];
        int c_lines = 0;

        char * a=lineptr;
        while(a != NULL){
            cmd_lines[c_lines] = a;
            strsep(&a, "&");

            c_lines++;
        }
        // for(int i=0; i<c_lines; i++){
        //     printf("%s\n",cmd_lines[i]);
        // }

        char ** my_argv = NULL;  
        int my_argc = 0;

        // printf("c_lines = %d", c_lines);
        for(int i=0; i<c_lines; i++){
            if(i>0){
                for(int i=0; i<my_argc; i++){
                    free(my_argv[i]);
                }
            }
            my_argc = 0;
            my_argv = malloc(100 * sizeof(char *));
            
            
            
            int take_args = 1;
            int c_files = 0;
            
            int error=0;
            
            char * filename=cmd_lines[i];
            
            strsep(&filename, ">"); //filename will be null if no > found
            char * a = filename;
            //search for additional >
            strsep(&a, ">");
            if(a!=NULL){
                print_error();
                continue;
            }
            
            a = filename;
            //check for multiple files
            strsep(&a," \t\n");
            if(a!=NULL){
                print_error();
                continue;
            }
            
            char cmd_line = cmd_lines[i] //strdup(cmd_lines[i]); //copies only the line before the ">" . TODO:CHeck if true

            char * beg = cmd_line; 
            char * ptr = cmd_line;

            while(beg[0] != '\0'){
                strsep(&ptr, " \n\t");
                if(beg[0] != '\0'){
                    my_argv[my_argc] = strdup(beg);
                    my_argc++;
                }
                beg = ptr;
            }
            
            my_argv[my_argc]=NULL;

            if(my_argc==0){
                continue;
            }

            //distinguish command
            if(strcmp(my_argv[0], "exit") == 0){
                if(my_argc!=1){ //no parameters
                    print_error();
                    // printf("argv[1]=%s",argv[1]);    
                }else{
                    exit(0);
                }
                    

            }else if(strcmp(my_argv[0], "cd") == 0){
                if(my_argc!=2){ //one parameter
                    print_error();
                        
                }else{
                    err = chdir(my_argv[1]);
                    if(err == -1){
                        print_error();
                    }
                }
                    

            }else if(strcmp(my_argv[0], "path") == 0){
                for (int i=0; i<c_paths; i++){
                    my_paths[i] = NULL;
                }

                c_paths = 0;
                while(c_paths<my_argc-1){
                    my_paths[c_paths] = strdup(my_argv[c_paths+1]);
                    c_paths++;
                }

                // for(int i=0; i<c_paths; i++){
                //     printf("path:%s\n",my_paths[i]);

                // }

            }else{
                if(c_paths==0){
                   print_error(); 
                   continue;
                }
                c_chd_ps++;
                f_rc = fork();
                
                if(f_rc==0){
                    if(filename != NULL){
                        close(STDOUT_FILENO);
                        fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU); 
                        dup2(fd, STDERR_FILENO);
                        
                    }

                    char* abs_pth = malloc(strlen(my_paths[i])+strlen(my_argv[0])+2); 
                    for(int i=0; i<c_paths; i++){
                        // printf("c_paths=%d\n", c_paths);
                        memset(abs_pth,'\0',strlen(my_paths[i])+strlen(my_argv[0])+2);
                        
                        strcpy(abs_pth,my_paths[i]);
                        //check if no / at the end
                        if(abs_pth[strlen(abs_pth)-1] != '/'){
                            strcat(abs_pth, "/");
                        }
                        strcat(abs_pth, my_argv[0]);
                        rc = access(abs_pth, X_OK);
                        // printf("este es er programa cojoneh:%s\n",abs_pth);
                        if(rc == -1){
                            
                            // fprintf(stderr, "access: %s",strerror(errno));
                            continue;
                        }else{
                            
                            
                            
                            rc =execv(abs_pth, my_argv);
                            exit(0);
                            printf("execv: %s\n", strerror(errno));
                            // if(rc==-1){
                            //     ;
                            // }
                        }
                        
                        
                    }
                    print_error();
                    exit(0);

                }


            }

        }
        

        if(f_rc>0){
                // while(c_chd_ps >0){
                //     while((rc = (int) wait(NULL))<0);
                    
                //     c_chd_ps--;
                //     // printf("child completed");
                // }

                // free(lineptr);

                // for(int i=0; i<c_paths; i++){
                //     free(my_paths[i]);
                // }   

                // free(my_paths);

                // //free memory from last line's my_argv (former my_argv have been free'd before)
                // for(int i=0; i<my_argc; i++){
                //     free(my_argv[i]);
                // }
                
                
                // printf("Done with the child process\n");
        }

        

        
        

    

        
        
    }
    
    
}