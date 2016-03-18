#ifndef LOG_H
#define LOG_H

#include <time.h>
#include <stdio.h>
#include <string.h>

void myLog(int who, int counter, char *what, char *fifo_name)
{

    int filedesc = open("tracker.log", O_WRONLY | O_APPEND);

    time_t now = time(NULL);

    char time[256];
    strcpy(time, ctime(&now));
    strtok(time, "\n");

    char quem[8];
    char second[94];
    char first[95];

    if(who == 1) {

        strcpy(quem, "Cliente");
        sprintf(second, "%-25s | %-7s | %-6d | %-25s | %-18s\n", time, quem, counter, what, fifo_name);
        write(filedesc, second, 94);
    }

    else if(who == 0){

        strcpy(quem, "Balcao");
        sprintf(second, "%-25s | %-7s | %-6d | %-25s | %-18s\n", time, quem, counter, what, fifo_name);
        write(filedesc, second, 94);
    
    }

    else if(who == 2){

        sprintf(first, "%-25s | %-7s | %-6s | %-25s | %-18s \n", "quando", "quem", "balcao", "o_que", "canal_criado/usado");
        write(filedesc, first, 95);

    }

    else{

        sprintf(first, "%-95s \n", "---------------------------------------------------------------------------------------------");
        write(filedesc, first, 95);
        close(filedesc);

    }

}

#endif /* LOG_H */

