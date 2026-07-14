// #include "./Clock/headers.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "DataStructures/PriorityQueue.h"

////////////////////////////////
Queue *processesQueue;
struct msgbuff processmsg;

void setProcessParameters(int id, int arr, int runningtime, int pri);
int createClockProcess();
int createSchedulerProcess();

int msgq_id;
char algo[2]; // string to be sent to scheduler as argument
char processesCount[2];
char quantum[5];

void clearResources(int);
int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);
    // TODO: Initialization

    FILE *file;
    char line[256];
    int processesNumber = 0;
    int id, arrivalTime, runningTime, priority;

    processmsg.mtype = 7; /* arbitrary value */

    // 1. Read the input files.
    // Open processes file
    file = fopen("processes.txt", "r");

    // Check if the file exist
    if (file == NULL)
    {
        perror("Error opening file");
        exit(-1);
    }

    // Read process data from the file
    processesQueue = createQueue();
    process *arrivedProcess = NULL;
    while (fgets(line, sizeof(line), file) != NULL)
    {
        // Check if the line starts with #
        if (line[0] == '#')
            continue; // Skip this line

        // Read process data from the line
        if (sscanf(line, "%d %d %d %d", &id, &arrivalTime, &runningTime, &priority) == 4)
        {
            arrivedProcess = createProcess(id, priority, arrivalTime, runningTime);
            normalQenqueue(processesQueue, arrivedProcess);
            printf("Process ID: %d, Arrival Time: %d, Running Time: %d, Priority: %d\n", id, arrivalTime, runningTime, priority);
            processesNumber++;
        }
    }
    sprintf(processesCount, "%d", processesNumber);

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    int algorithm;
    int RR;

    // Display menu to the user
    printf("Choose a scheduling algorithm:\n");
    printf("1. Non-preemptive Highest Priority First (HPF)\n");
    printf("2. Shortest Remaining Time Next (SRTN)\n");
    printf("3. Round Robin (RR)\n");
    printf("Enter your choice (1-3): ");

    scanf("%d", &algorithm);

    while (algorithm < 1 || algorithm > 3)
    {
        printf("Invalid choice. Please enter a number between 1 and 3.\n");
        scanf("%d", &algorithm);
    }

    if (algorithm == 3)
    {
        printf("Enter the time slice for Round Robin: ");
        scanf("%d", &RR);
        sprintf(quantum, "%d", RR);
    }

    printf("Chosen algorithm: %d\n", algorithm);
    sprintf(algo, "%d", algorithm);

    // 3. Initiate and create the scheduler and clock processes.

    //////////////////////////////////////////////////////////////////////////////////

    key_t key_id;
    int send_val;

    key_id = ftok("keyfile", 65);
    msgq_id = msgget(key_id, 0666 | IPC_CREAT);
    if (msgq_id == -1)
    {
        perror("Error in create");
        exit(-1);
    }

    ///////////////////////////////////////////////////////////////////////////

    int clckID = createClockProcess();
    int schedulerID = createSchedulerProcess();
    // printf("Scheduler id is: %d\n", schedulerID);

    ////////////////////////////////////////////////////////////////////////////////////////

    //   display(processesQueue);

    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this

    int x = getClk();
    printf("current time is %d\n", x);

    // TODO: Generation Main Loop

    ///////////////////////////////////////////////////////////////
    process *currp = peek(processesQueue);

    while (!isEmpty(processesQueue))
    {
        int currtime = getClk();
        if (currtime >= currp->arrivaltime)
        {
            process *temp = dequeue(processesQueue);
            currp = peek(processesQueue);
            setProcessParameters(temp->id, temp->arrivaltime, temp->runningtime, temp->priority);
            processmsg.mtype = schedulerID;
            send_val = msgsnd(msgq_id, &processmsg, sizeof(processmsg.arrivedProcess), !IPC_NOWAIT);
            //   kill (schedulerID, SIGUSR1);

            if (send_val == -1)
            {
                perror("sending error: ");
            }
            //  else
            //    printf("procees with priority: %d sent on queue\n", processmsg.arrivedProcess.priority);
        }
    }

    //////////////////////////////////////////////////////////////////////////////////

    // 5. Create a data structure for processes and provide it with its parameters.

    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources

    while (true)
        ;
    destroyClk(true);
    msgctl(msgq_id, IPC_RMID, (struct msqid_ds *)0);
    kill(getpgrp(), SIGKILL);

    fclose(file);
    return 0;
}

void clearResources(int signum)
{
    destroyClk(true);
    msgctl(msgq_id, IPC_RMID, (struct msqid_ds *)0);
    kill(getpgrp(), SIGKILL);
    // TODO: Clears all resources in case of interruption
}

void setProcessParameters(int id, int arr, int runningtime, int pri)
{
    processmsg.arrivedProcess.id = id;
    processmsg.arrivedProcess.arrivaltime = arr;
    processmsg.arrivedProcess.priority = pri;
    processmsg.arrivedProcess.runningtime = runningtime;
    processmsg.arrivedProcess.remainingtime = runningtime;
}

int createClockProcess()
{
    int clckID = fork();
    if (clckID == -1)
    {
        perror("error in fork");
        exit(-1);
    }
    else if (clckID == 0)
    {
        if (execl("./clk.out", "bin/clk.out", NULL) == -1)
        {
            perror("execl: ");
            exit(1);
        }
    }
    return clckID;
}

int createSchedulerProcess()
{

    int schedulerID = fork();
    if (schedulerID == -1)
    {
        perror("error in fork");
        exit(-1);
    }
    else if (schedulerID == 0)
    {
        if (execl("./scheduler.out", "scheduler.out", algo, processesCount, quantum, NULL) == -1)
        {
            perror("execl: ");
            exit(1);
        }
    }
    return schedulerID;
}
