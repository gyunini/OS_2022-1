//
//  main.c
//  pa1
//
//  Created by 이균 on 2022/04/09.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

int main(){
    int n, pid, fd[2];
    char line[100];
    pipe(fd);
    if((pid = fork()) == 0){
        close(fd[0]);
        dup2(fd[1],1);
        close(fd[1]);
        execl("/bin/who", "who", NULL);
    }
    else{
        close(fd[1]);
        dup2(fd[0],0);
        close(fd[0]);
        execl("/bin/wc", "wc", NULL);
        wait(NULL);
    }
    
    exit(0);
}
