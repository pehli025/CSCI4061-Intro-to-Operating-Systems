#include "commando.h"

cmd_t *cmd_new(char *argv[]) {
    cmd_t *copy = malloc(sizeof(cmd_t)); //Allocates for new cmd
    int i; //Counter for argv
    for(i=0; (i <= ARG_MAX-1) && (argv[i] != NULL); i++){   // make string copies
        copy->argv[i] = strdup(argv[i]);
    }

    //Default field values.
    strcpy(copy->name, argv[0]); 
    copy->finished = 0;
    snprintf(copy->str_status, 5, "INIT");
    copy->status = -1;
    copy->output = NULL;
    copy->output_size = -1;
    copy->out_pipe[0] = -1;
    copy->out_pipe[1] = -1;
    copy->pid = -1;
    copy->argv[i] = NULL;     
    return copy;
}

void cmd_free(cmd_t *cmd) { // deallocates cmd structure
    for(int i = 0; NAME_MAX; i++){
        if(cmd->argv[i] != NULL) {
            free(cmd->argv[i]); // goes through the array freeing each index
        } else { 
            break; 
        }
    }
    if(cmd->output != NULL){ // deallocate cmd buffer if not NULL
        free(cmd->output);
    }
    free(cmd); // deallocates cmd itself
}

void cmd_start(cmd_t *cmd) {
    snprintf(cmd->str_status, 4, "RUN"); // changes str_status field
    pipe(cmd->out_pipe); // creates a pope for out_pipe
    
    pid_t child = fork(); // forking
    cmd->pid = child; // pid field set to thhe child PID
    if(child == 0) { // child process
        dup2(cmd->out_pipe[PWRITE], STDOUT_FILENO); // directing standard ouptut
        execvp(cmd->name, cmd->argv); // executing name
        close(cmd->out_pipe[PREAD]); // closing pipe
    }
    close(cmd->out_pipe[PWRITE]); // closing parent pipe
}

void cmd_update_state(cmd_t *cmd, int block) {
    if(cmd->finished == 1) { 
        exit(0); // Do nothing
    } else { 
        int status;
        pid_t wp = waitpid(cmd->pid, &status, block); // using pid and block to set a non blocking or blocking    
        if(wp != 0){
            if(WIFEXITED(status)) { // checking return status
                cmd->finished = 1; // setting finished to 1
                cmd->status = WEXITSTATUS(status); // setting status
                sprintf(cmd->str_status, "EXIT(%d)", cmd->status); // printing exit status
            }
            cmd_fetch_output(cmd);
            printf("@!!! %s[#%d]: EXIT(%d)\n", cmd->name, cmd->pid, cmd->status); // printing status update
        }
    }
}

char *read_all(int fd, int *nread){
    int max_size = 4;
    int cur_pos = 0;
    char *buffer = malloc(max_size*sizeof(char)); // dynamically allocated buffer

    while(1) {
        if(cur_pos >= max_size){ // checks to see if the buffer needs to be increased.
            max_size = max_size*2;
            buffer = realloc(buffer, max_size);
        }
        int max_read = max_size - cur_pos;
        int nread = read(fd, buffer + cur_pos, max_read); // calling read
        if(nread != 0) {
            cur_pos += nread; 
        }
        if(nread == 0) {
            buffer[cur_pos] = '\0'; // puts null terminator at the end of the array
            break;
        }
        else if(nread == -1) {
            eprintf("%d failed to read\n", fd); // if nread returns the error for failed read print error
            exit(1);
        }
        
    }
    *nread = cur_pos;
    return buffer;
}

void cmd_fetch_output(cmd_t *cmd) {
    if(cmd->finished == 0){
        printf("%s [#%d] not finished yet\n", cmd->name, cmd->pid); // prints if not finished
    } else {
        int nread = 0; 
        cmd->output = read_all(cmd->out_pipe[PREAD], &nread); //capture output
        cmd->output_size = nread;
        close(cmd->out_pipe[PREAD]); // closes the pipe associated with the command after reading
    }
}

void cmd_print_output(cmd_t *cmd){
   if(cmd->output != NULL){
       write(STDOUT_FILENO, cmd->output, cmd->output_size);  // output not ready case
   } else {  // output ready, print out the data
       printf("%s[#%d] : output not ready\n", cmd->name, cmd->pid);
   }
}




