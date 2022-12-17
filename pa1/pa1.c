/**********************************************************************
 * Copyright (c) 2021
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>

#include <string.h>

#include "types.h"
#include "list_head.h"
#include "parser.h"
#include <sys/wait.h>
#define TOKEN_CNT 64

/***********************************************************************
 * run_command()
 *
 * DESCRIPTION
 *   Implement the specified shell features here using the parsed
 *   command tokens.
 *
 * RETURN VALUE
 *   Return 1 on successful command execution
 *   Return 0 when user inputs "exit"
 *   Return <0 on error
 */

static void cd(char *tokens[])
{
    if (tokens[1] != NULL && strcmp(tokens[1], "~") == 0){
        chdir(getenv("HOME"));
    }
    else if(tokens[1] == NULL){
        chdir(getenv("HOME"));
    }
    else
        chdir(tokens[1]);
}

void dump_stack(void);
void exec_history(char *tokens[]);


void do_pipe(char **tokens, int pipe_idx[], int pipe_cnt)
{
    int fd[2]; // [0] for read, [1] for write
    int pid;
    pipe(fd);
    
    // this child is generating output to the pipe
    if((pid = fork()) == 0){ // child
        // attach stdout to the left side of pipe
        // and inherit stdin and stdout from parent
        dup2(fd[1], 1); // stdout -> fd[1]
        close(fd[0]);
        execvp(tokens[pipe_idx[0]], &tokens[pipe_idx[0]]);
        
    }
    else if ((pid = fork()) == 0){// this child is consuming input from the pipe
        // attach stdin to the right side of pipe
        // and inherit stdout and stderr from parent
        dup2(fd[0], 0); // stdin -> fd[0]
        close(fd[1]);
        execvp(tokens[pipe_idx[1]], &tokens[pipe_idx[1]]);
    }
    else if(pid > 0){
        close(fd[0]);
        close(fd[1]);
        wait(0);
        wait(0);
    }
    else{
        fprintf(stderr, "Unable to execute %s\n", tokens[0]);
    }
}

static int run_command(int nr_tokens, char *tokens[])
{
    int pid, status, ret;
    bool has_pipe = false;
    int pipe_cnt = 0;
    int len = 0;
    int pipe_idx[TOKEN_CNT] = {0, };
    for (int i = 0; tokens[i] != NULL; i++, len++){  // 파이프 있을 경우 token NULL로 변경
        if (strcmp(tokens[i], "|") == 0) {
          tokens[i] = NULL;
          pipe_idx[++pipe_cnt] = i+1; // pipdIdx에 pipe로 구분되는 token들 시작 index 저장
          has_pipe = true;
        }
    }
    if (has_pipe) {
        do_pipe(tokens, pipe_idx, pipe_cnt);
    }
    else{
        if (strcmp(tokens[0], "exit") == 0)
            return 0;
        
        else if (strcmp(tokens[0], "cd") == 0){
            cd(tokens);
        }
        else if (strcmp(tokens[0], "history") == 0){
            dump_stack();
        }
        else if (strcmp(tokens[0], "!") == 0){
            exec_history(tokens);
        }
        else{
            pid = fork();
            if (pid == 0){
                ret = execvp(tokens[0], tokens);
                if(ret == -1){
                    fprintf(stderr, "Unable to execute %s\n", tokens[0]);
                }
            }
            else if(pid > 0){
                waitpid(pid, &status, 0);
            }
            else{
                fprintf(stderr, "Unable to execute %s\n", tokens[0]);
                return -EINVAL;
            }
        }
    }
    return 1;
}


/***********************************************************************
 * struct list_head history
 *
 * DESCRIPTION
 *   Use this list_head to store unlimited command history.
 */
LIST_HEAD(history);


/***********************************************************************
 * append_history()
 *
 * DESCRIPTION
 *   Append @command into the history. The appended command can be later
 *   recalled with "!" built-in command
 */

/* Declaration for the stack instance defined in pa0.c */
//extern struct list_head history;

/* Entry for the stack */
struct entry {
    struct list_head list;
    char *string;
};

void push_stack(char *string)
{
    /* TODO: Implement this function */
    struct entry* ptr = (struct entry*)malloc(sizeof(struct entry));
    //printf("%d\n", strlen(string));
    ptr->string = (char*)malloc(strlen(string)+1);
    strncpy(ptr->string, string, strlen(string)+1);
    list_add(&(ptr->list), &history);
}

int pop_stack(char *buffer)
{
    /* TODO: Implement this function */
    if(!list_empty(&history)){
        struct entry *new_entry = list_entry(history.next, struct entry, list);
        strcpy(buffer, new_entry->string);
        list_del(history.next);
        free(new_entry->string);
        free(new_entry);
        return 0;
    }
    else
        return -1; /* Must fix to return a proper value when @stack is not empty */
}

void dump_stack(void)
{
    /* TODO: Implement this function */
    int index = 0;
    struct entry  *datastructureptr = NULL ;
    list_for_each_entry_reverse(datastructureptr, &history, list)
    {
        //fprintf(stderr, "%s\n", datastructureptr->string);
        fprintf(stderr, "%2d: %s", index, datastructureptr->string);
        index += 1;
    }
    /* fprintf(stderr, "%s\n", "0xdeadbeef");  Example.
                                            Print out values in this form */
}

static int __process_command(char * command);

void exec_history(char *tokens[])
{
    /* TODO: Implement this function */
    int index = 0;
    struct entry  *datastructureptr = NULL ;
    char cpString[MAX_COMMAND_LEN] = { '\0' };
    list_for_each_entry_reverse(datastructureptr, &history, list)
    {
        //fprintf(stderr, "%2d: %s", index, datastructureptr->string);
        if(atoi(tokens[1]) == index){
            //execvp(datastructureptr->string, tokens);
            strcpy(cpString,datastructureptr->string);
            __process_command(cpString);
            break;
        }
        index += 1;
    }
}

static void append_history(char * const command)
{
    push_stack(command);
}


/***********************************************************************
 * initialize()
 *
 * DESCRIPTION
 *   Call-back function for your own initialization code. It is OK to
 *   leave blank if you don't need any initialization.
 *
 * RETURN VALUE
 *   Return 0 on successful initialization.
 *   Return other value on error, which leads the program to exit.
 */
static int initialize(int argc, char * const argv[])
{
    return 0;
}


/***********************************************************************
 * finalize()
 *
 * DESCRIPTION
 *   Callback function for finalizing your code. Like @initialize(),
 *   you may leave this function blank.
 */
static void finalize(int argc, char * const argv[])
{

}


/*====================================================================*/
/*          ****** DO NOT MODIFY ANYTHING BELOW THIS LINE ******      */
/*          ****** BUT YOU MAY CALL SOME IF YOU WANT TO.. ******      */
static int __process_command(char * command)
{
    char *tokens[MAX_NR_TOKENS] = { NULL };
    int nr_tokens = 0;

    if (parse_command(command, &nr_tokens, tokens) == 0)
        return 1;

    return run_command(nr_tokens, tokens);
}

static bool __verbose = true;
static const char *__color_start = "[0;31;40m";
static const char *__color_end = "[0m";

static void __print_prompt(void)
{
    char *prompt = "$";
    if (!__verbose) return;

    fprintf(stderr, "%s%s%s ", __color_start, prompt, __color_end);
}

/***********************************************************************
 * main() of this program.
 */
int main(int argc, char * const argv[])
{
    char command[MAX_COMMAND_LEN] = { '\0' };
    int ret = 0;
    int opt;

    while ((opt = getopt(argc, argv, "qm")) != -1) {
        switch (opt) {
        case 'q':
            __verbose = false;
            break;
        case 'm':
            __color_start = __color_end = "\0";
            break;
        }
    }

    if ((ret = initialize(argc, argv))) return EXIT_FAILURE;

    /**
     * Make stdin unbuffered to prevent ghost (buffered) inputs during
     * abnormal exit after fork()
     */
    setvbuf(stdin, NULL, _IONBF, 0);

    while (true) {
        __print_prompt();

        if (!fgets(command, sizeof(command), stdin)) break;

        append_history(command);
        ret = __process_command(command);

        if (!ret) break;
    }

    finalize(argc, argv);

    return EXIT_SUCCESS;
}
