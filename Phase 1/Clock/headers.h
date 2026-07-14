#include <stdio.h> //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////////

typedef short bool;
#define true 1
#define false 0

#define SHKEY 300
//////////////
#define GEN_SCH_SHmKEY 400
/////////////

typedef struct process
{
    int arrivaltime;
    int priority;
    int runningtime;
    int remainingtime;
    int id;
    int realPid;
    int lastRunningClk;
    int starttime;
    char state[10];
    int laststoptime;
    int waitingtime;
} process;

// Function to create a new process node
process *createProcess(int processID, int priority, int arrivaltime, int runningtime)
{
    process *newProcess = (process *)malloc(sizeof(process));
    if (newProcess == NULL)
    {
        printf("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newProcess->id = processID;
    newProcess->priority = priority;
    newProcess->arrivaltime = arrivaltime;
    newProcess->runningtime = runningtime;
    newProcess->remainingtime = runningtime; // id got from input and will be printed in output.  ex: 1,2,3,..
    newProcess->realPid = 0;                 // pid for scheduler to send signals to processes if needed
    newProcess->lastRunningClk = arrivaltime;
    strcpy(newProcess->state, "started");
    newProcess->starttime = arrivaltime;
    newProcess->laststoptime = arrivaltime;
    newProcess->waitingtime=0;

    return newProcess;
}

struct msgbuff
{
    long mtype;
    process arrivedProcess;
};

struct sch_proc_buff
{
    long mtype;
    int currtime;
};

///==============================
// don't mess with this variable//
int *shmaddr; //
//===============================

int getClk()
{
    return *shmaddr;
}

/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
 */
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0666 | IPC_CREAT);
    while ((int)shmid == -1)
    {
        // Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0666);
    }
    shmaddr = (int *)shmat(shmid, (void *)0, 0);
    if (shmaddr == (void *)-1)
    {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }

    // Print a message to indicate successful initialization
    printf("Clock initialized.\n");
}

/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
 */

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}
