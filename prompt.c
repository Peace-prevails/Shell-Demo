//
//
//  Created by Peace-prevails on 2023/2/9.
//

#include "prompt.h"
volatile sig_atomic_t sig_flag = 0;
void handler();
int build_in(char* command,char** args,int arg_num);
int check_command(char * command,char** arguments,int arg_num);
int cus_cd(char * dir);
void cus_exit(int* jobs_index_need_free,int max_susp_space,char ** susp_jobs);
void cus_jobs(char ** susp_jobs);
int cus_fg(char ** susp_jobs,pid_t susp_id[],int ind,int* jobs_index,int arg_num);

int start_prompt(void){
   
    char *suspend_jobs[100]={NULL};
    pid_t suspend_jobs_id[100]={0};
    int* jobs_index=malloc(sizeof(int));
    *jobs_index=0;
    int max_suspended_jobs=0;
    while(1){
//        printf("jobs index: %d\n",*jobs_index);
        if(max_suspended_jobs<(*jobs_index)){
            max_suspended_jobs=(*jobs_index);
        }
//        printf("max space allocated%d\n",max_suspended_jobs);
        //read input
        signal(SIGINT, handler);
        signal(SIGTSTP, handler);
        signal(SIGQUIT, handler);
        char *b =NULL;
        size_t len = 0;
        ssize_t readin;
        sig_flag=0;
        
        char cwd[FILENAME_MAX+1];
        if (getcwd(cwd, sizeof(cwd))!=NULL){
            if (strcmp(cwd,"/")!=0){
                char* lastSlash=strrchr(cwd,'/');
                char* baseName=lastSlash?lastSlash+1:cwd;
                printf("[nyush %s]$ ",baseName);
                fflush(stdout);
            }
            else{
                printf("[nyush %c]$ ",'/');
                fflush(stdout);
            }
        }
//            if (sig_flag==1){
//                printf("top\n");
//                continue;
//            }
            readin = getline(&b, &len, stdin);
            
            if (sig_flag==1){;
                continue;
            }
            
            if (readin == -1) {
                free(b);
                exit(0);
            }
            
            char* command=NULL;
            char* token=NULL;
            char* arguments[30]={NULL};
            int arg_count=0;
            char* line=malloc(strlen(b)+1);
            char* record=malloc(strlen(b)+1);
            strcpy(line, b);
            strcpy(record, b);
            int rec_len = strlen(record);
            if (record[rec_len-1]=='\n') {
//                printf("record changing\n");
                record[rec_len - 1] = '\0';
            }
//            printf("record:%s\n",record);
            free(b);
            
            token = strtok(line, " ");
            while (token!=NULL){
                if (arg_count==0){
                    command=token;
                }
                
                token = strtok(NULL, " ");
                if (token!=NULL){
                    int len = strlen(token);
                        if (token[len-1]=='\n') {
                            token[len - 1] = '\0';
                        }
                }
                arguments[arg_count]=token;
                arg_count++;
                
            }
            arg_count--;
            if (arg_count==0){
                int len = strlen(line);
                command=line;
                command[len-1]='\0';
            }
            if(check_command(command,arguments,arg_count)==0){
                fprintf(stderr, "Error: invalid command\n");
                continue;
            }
            
           
            
            char *arg[100]={NULL};
            char* output_op=NULL;
            char* output_f=NULL;
            char* input_op=NULL;
            char* input_f=NULL;
            int i=0;
            int arg_boundary=0;
            int arg_stop_flag=0;
            arg[0]=malloc(strlen(command) + 1);
            strcpy(arg[0], command);
            while(i<arg_count){
                if(strcmp(">",arguments[i])==0||strcmp(">>",arguments[i])==0){
                    int pipefd[2];
                    pipe(pipefd);
                    output_op = malloc(strlen(arguments[i]) + 1);
                    output_f = malloc(strlen(arguments[i+1]) + 1);
                    strcpy(output_op,arguments[i]);
                    strcpy(output_f,arguments[i+1]);
                    arg_stop_flag=1;
//                    break;
                }
                else if(strcmp("<",arguments[i])==0 && i!=0){
                    int pipefd[2];
                    pipe(pipefd);
                    input_op = malloc(strlen(arguments[i]) + 1);
                    input_f = malloc(strlen(arguments[i+1]) + 1);
                    strcpy(input_op,arguments[i]);
                    strcpy(input_f,arguments[i+1]);
                    arg_stop_flag=1;
//                    break;
                }
                else if(arg_stop_flag==0){
                    arg[i+1]=malloc(strlen(arguments[i]) + 1);
                    strcpy(arg[i+1],arguments[i]);
                    arg_boundary++;
                }
                i++;
            }
            arg[arg_boundary+1]=NULL;
            
      
        if(input_op!=NULL && input_f==NULL){fprintf(stderr, "Error: invalid command\n");
        continue;}
        if(output_op!=NULL && output_f==NULL){fprintf(stderr, "Error: invalid command\n");
        continue;}
        
//        int test_ind=0;
//        while(arg[test_ind]!=NULL){printf("arg in arg[%d] is %s\n",test_ind,arg[test_ind]);test_ind++;}
//            int k = 0;
//            while (arg[k] != NULL) {
//                printf("arg array[%d]:%s\n",k,arg[k]);
//
//                k++;
//            }
            int fd=-1;

//            free(line);
            if (strcmp(command,"fg")==0){
//                    int sta=0;
                    cus_fg(suspend_jobs, suspend_jobs_id, atoi(arguments[0]),jobs_index,arg_count);
//                    if (sta==1){
//                        jobs_index--;
//                    }
                free(line);
                 
            }
            else if(strcmp(command, "cd")==0){
                build_in(command,arguments,arg_count);
                free(line);
            }
            else if (strcmp(command,"exit")==0){
//                printf("this command: %s\n",command);
                int susp_j_len=0;
                free(line);
                while(suspend_jobs[susp_j_len]!=NULL){
                    susp_j_len++;
                }
                if(arg_count!=0){
                    fprintf(stderr, "Error: invalid command\n");
                }
                else if(susp_j_len!=0){
                    fprintf(stderr, "Error: there are suspended jobs\n");
                }
                else
                {
                    cus_exit(jobs_index,max_suspended_jobs,suspend_jobs);
                }
            }
            else if (strcmp(command,"jobs")==0){
                build_in(command,suspend_jobs, arg_count);
                free(line);
            }
            
            else
            {
                
             
                pid_t new_proc=fork();
                
                if (new_proc==0){
//                    printf("\nentering child process\n\n");
                    signal(SIGINT, SIG_DFL);
                    signal(SIGTSTP, SIG_DFL);
                    signal(SIGQUIT, SIG_DFL);
                    
//                    printf("child proc:%d\n",getpid());
                    int len = strlen(command);
                    if (command[len-1]=='\n') {
                        command[len - 1] = '\0';
                            }
//                    printf("command[0]:%c\n",command[0]);
                    char ext_program[100];
                    int is_bin_prg=1;
                    for(int ind=0;ind<len;ind++){
                        if (command[ind]=='/'){
                            is_bin_prg=0;
                        }
                    }
                    if (is_bin_prg==1){
//                        printf("hi,is bin program with command %s\n",command);
                        strcpy(ext_program, "/usr/bin/");
                        strcat(ext_program, command);
                        
                    }
                    else {
                        strcpy(ext_program, command);
                    }
                   
//                    printf("command path:%s,command length %d\n",ext_program,strlen(command));
//                    printf("argument[0]:%s\n",arguments[0]);
//                    printf("ext_prog:%s\n",ext_program);
                    if (arguments[0]==NULL){
//                        printf("running bin program %s\n",ext_program);
                        execl(ext_program,command,NULL);
                    }
                    else{
                        if (input_op!=NULL || output_f!=NULL){
                            if (input_op != NULL){
//                                printf("input redirection\n");
                                int fd=-1;
                                fd = open(input_f, O_RDONLY);
                                dup2(fd, STDIN_FILENO);
                                close(fd);
                                free(input_f);
                                free(input_op);
//                                execv(ext_program,arg);
                            }
                            if (output_op != NULL)
                            {
    //                            printf("this is output_file program\n");
    //                            int fd=-1;
    //                            if (strcmp(output_op, "<") == 0){
    //                                fd = open(output_f, O_RDONLY);
    //                                dup2(fd, STDIN_FILENO);
    //                                close(fd);
    //                            }
                                    if (strcmp(output_op, ">") == 0) {
                                        // Overwrite existing file or create if it does not exist
                                        fd = open(output_f, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                                    } else if (strcmp(output_op, ">>") == 0) {
                                        // Append to existing file or create if it does not exist
                                        fd = open(output_f, O_WRONLY | O_CREAT | O_APPEND, 0666);
                                    }
                                    if (fd < 0) {
                                        // Error opening file
                                        perror("can't create or open file");
                                        exit(EXIT_FAILURE);
                                    }
                                    // Redirect stdout to file
                                    dup2(fd, STDOUT_FILENO);
                                    close(fd);
                
                                free(output_f);
                                free(output_op);
//                                execv(ext_program,arg);
                            }
                            execv(ext_program,arg);
                        }
                            
                        
                        
                        if(strcmp(arg[1],"<")==0)
                        {
//                            printf("opening %s \n",arg[2]);
                            fd = open(arg[2], O_RDONLY);
                            if (fd == -1) {
                                fprintf(stderr,"Error: invalid file\n");
                            }
                            if (dup2(fd, STDIN_FILENO) == -1)
                            {
//                                fprintf(stderr, "Error: invalid program\n");
                                exit(EXIT_FAILURE);
                            }

                            // Close the file descriptor for the input file
                            close(fd);
                            char* input_arg[]={command,NULL};
                            execvp(command, input_arg);
                        }

                        else{
                            execv(ext_program,arg);
                        }
                        
                           
                    }
                    
                    fprintf(stderr, "Error: invalid program\n");
                    exit(0);
                    
                }
                else{
                    
                    int status;
                    waitpid(new_proc, &status, WUNTRACED);
//
//                    if (WIFSIGNALED(status)!=0){
//                        printf("signaled\n");
//                        printf("\n");
//                    }
                    if (WIFSTOPPED(status)){
//                        printf("\n");
                        
                        suspend_jobs[*jobs_index] = malloc(strlen(record) + 1);//free it when resume process
                        strcpy(suspend_jobs[*jobs_index], record);
//                        printf("susp[%d] is %s\n",*jobs_index,suspend_jobs[*jobs_index]);
                        suspend_jobs_id[*jobs_index]=new_proc;
                        (*jobs_index)++;
                        free(record);
                    }
//                    sig_flag=0;
                    // Free memory allocated for command
//                    free(line);
                    if(arg_count==0){
                        free(arg[0]);
                    }
                    else{

                        int i = 0;
                        while (arg[i]!=NULL) {
                            free(arg[i]);
                            i++;
                        }
                        free(arg[i]);
                    }

                    free(line);
//                    close(fd);
                }
            }
            
//        }
    }
    return 0;
}

void handler(){
//    printf("this is a parent signal handler!!!!!\n");
//    fflush(stdin);
//    printf("\n");
    sig_flag = 1;
    return;
}
int cus_fg(char ** susp_jobs, pid_t susp_id[], int ind, int* jobs_index,int arg_num) {
    if (arg_num>1|| arg_num==0){
        fprintf(stderr, "Error: invalid command\n");
        return 0;
        
    }
    // Check if the specified index is valid
    if (ind < 1 || ind > *jobs_index) {
        fprintf(stderr, "Error: invalid job\n");
        return 0;
    }

    // Send the SIGCONT signal to the specified process
    kill(susp_id[ind - 1], SIGCONT);
//    printf(("sending sigcont to %d \n"),susp_id[ind-1]);
//    cus_jobs(susp_jobs);
//     Wait for the process to terminate
//    printf("waiting for :%d\n",susp_id[ind - 1]);
    
//    printf("susp_pid:%d\n",susp_id[0]);
    // Remove the job from the suspended jobs array
    
    
    int status;
    if (waitpid(susp_id[ind - 1], &status, WUNTRACED) == -1) {
//        perror("waitpid");
        return 0;
    }
    if (WIFSTOPPED(status)){ //if stopped, add it back to the end of the list
//        printf("\n");
        susp_jobs[*jobs_index] = malloc(strlen(susp_jobs[ind - 1]) + 1);
        strcpy(susp_jobs[*jobs_index],susp_jobs[ind - 1] );
        susp_id[*jobs_index]=susp_id[ind - 1];
        (*jobs_index)++;
        
        for (int i = ind - 1; i < (*jobs_index) - 1; i++) {
            susp_jobs[i] = susp_jobs[i + 1];
            susp_id[i] = susp_id[i + 1];
        }
        susp_jobs[(*jobs_index) - 1] = NULL;
        susp_id[(*jobs_index) - 1] = 0;
        (*jobs_index) --;
        
        
    }
    else if (WIFEXITED(status)){ //if exited, remove it from the list
//        printf("finished\n");
        for (int i = ind - 1; i < (*jobs_index) - 1; i++) {
            susp_jobs[i] = susp_jobs[i + 1];
            susp_id[i] = susp_id[i + 1];
        }
        susp_jobs[(*jobs_index) - 1] = NULL;
        susp_id[(*jobs_index) - 1] = 0;
        (*jobs_index) --;
    }

    return 0;
}
void cus_jobs(char ** susp_jobs){
    int ind=0;
    
    while(susp_jobs[ind]!=NULL){
//        printf("cusjobs[%s]\n",susp_jobs[ind]);
        ind++;
    }
    for(int i=0;i<ind;i++){
        printf("[%d] %s\n",i+1,susp_jobs[i]);
    }
    return;
}

int cus_cd(char* dir){
//    printf("directory:%s\n",dir);
    if (chdir(dir) != 0) {
        fprintf(stderr, "Error: invalid directory\n");
    }
    return 0;
}
void cus_exit(int* jobs_index_need_free,int max_susp_space,char ** susp_jobs){
//    free(line);
    free(jobs_index_need_free);
    for(int i=0;i<max_susp_space;i++){
        free(susp_jobs[i]);
//        free(&susp_id[i]);
    }
    
    exit(0);
}
int build_in(char* command,char** args,int arg_num){
//    printf("command: %s\n",command);
    if(strcmp(command, "cd")==0){
        if(arg_num!=1){
            fprintf(stderr, "Error: invalid command\n");
        }
        else{
            cus_cd(args[0]);
        }
    }
    else if(strcmp(command, "jobs")==0){
        if (arg_num!=0){
            fprintf(stderr, "Error: invalid command\n");
        }
        else{
            cus_jobs(args);//args in this function is b,the whole line,not just args.
        }
        

    }
    else{
        
        return 0;
    }
    return 1; // 1 means successfully done one build_in command.
}
int check_command(char * command,char** arguments,int arg_num){
    int flag=0;
    for(int i=0;i<arg_num;i++){
        if (strcmp(arguments[i],"<")==0 ||strcmp(arguments[i],">>")==0 ||strcmp(arguments[i],"|")==0 ||strcmp(arguments[i],">")==0 ||strcmp(arguments[i],"<<")==0){
            flag=1;
        }
    }
    if (strcmp(command, "cat")==0 || flag==1 ){
        if (arg_num==1 && strcmp(arguments[0],"<")==0){
            return 0;
        }
        else if (arg_num==1 && strcmp(arguments[0],">")==0)
        {
            return 0;
        }
        else if (arg_num==1 && strcmp(arguments[0],"|")==0)
        {
            return 0;
        }
        else if (strcmp(arguments[0],"<<")==0)
        {
            return 0;
        }
        return 1;
    }
    else if (strcmp(command, "|")==0){
        return 0;
    }
    else if (strcmp(command, "<")==0){
        return 0;
    }
    else if (strcmp(command, "<<")==0){
        return 0;
    }
    else if (strcmp(command, ">")==0){
        return 0;
    }
    else{
        return 1;
        
    }

}
