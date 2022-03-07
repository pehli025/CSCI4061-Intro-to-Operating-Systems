#include "commando.h"

int main(int argc, char **argv){
    setvbuf(stdout, NULL, _IONBF,0);
    int echo = 0;

    if(argc > 1 && strcmp("-echo", argv[1])==0){
        echo = 1;
    }
    char* buffer = malloc(sizeof(char)*MAX_LINE);
    char* tokens[ARG_MAX];
    int num_tok = 0;

    cmd_t* _cmd =  malloc(sizeof(cmd_t));
    cmdcol_t* _cmdcol = malloc(sizeof(cmdcol_t));

    _cmdcol->size=0;
    int size = _cmdcol->size;

    while(1){
        printf("@> ");
        fgets(buffer,MAX_LINE, stdin);
        parse_into_tokens(buffer, tokens, &num_tok);
        if(feof(stdin)){
            free(buffer);
            while(num_tok != -1){
                tokens[num_tok+1] = NULL;
                num_tok--;
            }
            free(_cmdcol);
            free(_cmd);
            printf("\nEnd of input");
            break;
        }
        if(tokens[0] == NULL) { 
                printf("\n");
        }
        else if (strcmp(tokens[0], "help") == 0){
            printf("help\n");
            printf("COMMANDO COMMANDS\n");
            printf("help               : show this message\n");
            printf("exit               : exit the program\n");
            printf("list               : list all jobs that have been started giving information on each\n");
            printf("pause nanos secs   : pause for the given number of nanseconds and seconds\n");
            printf("output-for int     : print the output for given job number\n");
            printf("output-all         : print output for all jobs\n");
            printf("wait-for int       : wait until the given job number finishes\n");
            printf("wait-all           : wait for all jobs to finish\n");
            printf("command arg1 ...   : non-built-in is run as a job\n");
        } 
        else if (strcmp(tokens[0], "list") == 0) {
            printf("list\n");
            cmdcol_print(_cmdcol);
        } 
        else if(strcmp(buffer,"\r\n") == 0) {
            if(echo == 1) { printf("\n"); }
        }
        else if(strcmp(tokens[0], "pause") == 0) {
            printf("pause %s %s\n", tokens[1], tokens[2]);
            pause_for(atoi(tokens[1]), atoi(tokens[2]));
            for(int i = 0; i <  size; i++){
                if(_cmdcol->cmd[i]->finished != 1){
                    i++;
                }
            }
        }
        else if(strcmp(tokens[0], "output-for") == 0) {
            printf("output-for %s\n", tokens[1]);
            printf("@<<< Output for %s[#%d] (%d bytes): \n",
                _cmdcol->cmd[atoi(tokens[1])]->name,_cmdcol->cmd[atoi(tokens[1])]->pid, _cmdcol->cmd[atoi(tokens[1])]->output_size);
            printf("----------------------------------------\n");
            cmd_print_output(_cmdcol->cmd[atoi(tokens[1])]);
            printf("----------------------------------------\n");
        }

        else if(strcmp(tokens[0], "output-all") == 0) {
            printf("output-all\n");
            for(int i = 0; i < size; i++) {
                printf("@<<< Output for %s[#%d] (%d bytes): \n",
                _cmdcol->cmd[i]->name,_cmdcol->cmd[i]->pid, _cmdcol->cmd[i]->output_size);
                printf("----------------------------------------\n");
                cmd_print_output(_cmdcol->cmd[i]);
                printf("----------------------------------------\n");
            }
        }

        else if(strcmp(tokens[0], "wait-for") == 0) {
            printf("wait-for %s\n", tokens[1]);
            if(_cmdcol->cmd[atoi(tokens[1])]->finished != 1) {
                cmd_update_state(_cmdcol->cmd[atoi(tokens[1])], DOBLOCK);
            }
        }

        else if(strcmp(tokens[0], "wait-all") == 0 ) {
            printf("wait-all\n");
            for(int i = 0; i < size; i++){
                if(_cmdcol->cmd[i]->finished != 1) { cmd_update_state(_cmdcol->cmd[i], DOBLOCK); }
            }
        }
        else if(strcmp(tokens[0], "exit") == 0) {
            printf("exit\n");
            free(buffer);
            while(num_tok != -2) {
                tokens[num_tok+1] = NULL;
                num_tok--;
            }
            free(_cmd);
            cmdcol_freeall(_cmdcol);
            free(_cmdcol);
            break;
        } else {
            for(int i = 0; i < num_tok; i++) {
                printf("%s ", tokens[i]);
            }
            printf("\n");
            cmd_t *new = cmd_new(tokens);
            cmd_start(new);
            cmdcol_add(_cmdcol, new);
            size++;
        }
        for(int i = 0; i < size; i++){
            if(_cmdcol->cmd[i]->finished != 1) { cmd_update_state(_cmdcol->cmd[i], NOBLOCK); }
        }
    }
    exit(1);
  }