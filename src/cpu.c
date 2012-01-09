#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "ncurses_ui.h"
#include "cpu.h"
#include "instruction_issue_unit.h"

//These two, are used to force the clock rate uppon the units
pthread_mutex_t clk_mtx;
pthread_cond_t pulse;

//Used to freeze the CPU
pthread_mutex_t pause_mtx = PTHREAD_MUTEX_INITIALIZER;

void shutdown_cpu();
void pause_cpu();
void *cpu_clock(void *arg);

int main(int argc, char **argv)
{
    FILE *stderr_out;
    pthread_t clk, iiu;
    char **titles = (char **)calloc(4, sizeof(char *));
    titles[0] = "Instruction Issue unit";
    titles[1] = "Data cache unit";
    titles[2] = "Integer unit";
    titles[3] = "Float unit";

    /* Redirect stderr to a file */
    stderr_out = freopen("stderr.out", "w", stderr);
    setbuf(stderr_out, NULL);

    /* Init User interface */
    init_ncurses(4, titles);
    //27=Esc
    init_key_ctrls(3, 'r', refresh_all, 27, shutdown_cpu, ' ', pause_cpu);

    /* Init  variables */
    pthread_mutex_init(&clk_mtx, NULL);
    pthread_mutex_init(&pause_mtx, NULL);
    pthread_cond_init(&pulse, NULL);

    /* Start the threads */
    pthread_create(&clk, NULL, cpu_clock, NULL);
    pthread_create(&iiu, NULL, instruction_issue, NULL);

    pthread_join(clk, NULL);
    pthread_join(iiu, NULL);
    
    return 0;
}

void shutdown_cpu()
{
    //free resources
    fclose(stderr);
    end_ncurses();
    pthread_mutex_destroy(&pause_mtx);
    pthread_mutex_destroy(&clk_mtx);
    pthread_cond_destroy(&pulse);

    printf("Shutdown CPU\n");
    exit(0);
}

void pause_cpu()
{
    static int state;
    
    if (state == 0) { //pause
        pthread_mutex_lock(&pause_mtx);
    } else { //unpause
        pthread_mutex_unlock(&pause_mtx);
    }
    state = (state + 1) % 2;
}

void *cpu_clock(void *arg)
{
    static int cur_clk;
    pthread_mutex_lock(&clk_mtx);
    while(1) {
        pthread_mutex_unlock(&pause_mtx);        

        sleep(1);
        pthread_cond_broadcast(&pulse);
        fprintf(stderr, "tick [%d]\n", cur_clk++);

        pthread_mutex_lock(&pause_mtx);        
    }
}

void clk_cycle()
{
    pthread_cond_wait(&pulse, &clk_mtx);
}