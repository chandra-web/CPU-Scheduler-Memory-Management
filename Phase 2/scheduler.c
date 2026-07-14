#include "DataStructures/PriorityQueue.h"
#include <sys/file.h>
#include <math.h>
#include <stdio.h>
#define MEMORY_SIZE 1024


PriorityQueue *PQ = NULL;
Queue *Q = NULL;
Queue *finishedQueue = NULL;

int proc_msgq_id;

// set to zero when it receives a termination signal from a process
int flag = 1;
int received = 1;
int quantum;

int processCount; // to check if all processes finished or other processes were not sent yet
int algorithm;
char salgo[2];
int currentProcessCount;
key_t sch_key_id;
int sch_rec_val, sch_msgq_id;
struct msgbuff SCH_message;

struct sch_proc_buff proc_buff;

int children_shmid;
int *children_shmaddr;

int totalWaitingTime = 0;
float totalWTA = 0;
int totalRT = 0;

extern FILE *schedulerLog;

Queue *waitingQueue;
MemoryBlock *memory_block;

process *runningProcess = NULL;

/////// FUNCTIONS ////////

int forkNewProcess(char *runtime);
void getAlgorithm();
void connectWithGenerator();
void finishedPhandler(int signum);
void sigtermhandler(int signum);
void processReceivedSignal(int signum);
void RRScheduler(int quantum);
process *initProcess();

void SRTN();
void SRTNaddprocess();

void HPF();
void HPFaddprocess();
void printProcessState();
void printProcessFinish();

int main(int argc, char *argv[])
{
  initClk();

  signal(SIGTERM, sigtermhandler);        // to free the allocated memory
  signal(SIGUSR1, finishedPhandler);      // to recieve that a process has finished its execution
  signal(SIGUSR2, processReceivedSignal); // to check if a process received a continue signal
  schedulerLog = fopen("scheduler.log", "w");
  algorithm = atoi(argv[1]);
  sprintf(salgo, "%d", algorithm);
  processCount = atoi(argv[2]);
  currentProcessCount = processCount;
  quantum = atoi(argv[3]);
  finishedQueue = createQueue();
  connectWithGenerator();

  proc_msgq_id = msgget(4000, 0666 | IPC_CREAT);
  if (proc_msgq_id == -1)
  {
    perror("Error in create");
    exit(-1);
  }
  else
    printf("Message queue id = %d\n", proc_msgq_id);

  waitingQueue = createQueue();
  memory_block = createMemoryBlock(0, MEMORY_SIZE, NULL);

  getAlgorithm();
  printf("Freeing Memory\n");
  freeAllMemory(memory_block);
  destroyClk(true);
  fclose(schedulerLog);
  kill(getppid(), SIGINT);
  return 0;
}

void SRTN()
{
  PQ = createPriorityQueue();
  while (processCount > 0)
  {
    sch_rec_val = msgrcv(sch_msgq_id, &SCH_message, sizeof(SCH_message.arrivedProcess), getpid(), IPC_NOWAIT);
    if (sch_rec_val != -1)
    {
      SRTNaddprocess();
    }
    if (runningProcess == NULL && !PQisEmpty(PQ))
    {
      runningProcess = PQpeek(PQ);

      ///////////////////////////// for message queue method //////////////////////////////
      if (runningProcess->runningtime != runningProcess->remainingtime) // if it is the first run for the process, the handler is not attatched to the signal yet
      {
        proc_buff.currtime = getClk();
        proc_buff.mtype = runningProcess->realPid;
        msgsnd(proc_msgq_id, &proc_buff, sizeof(proc_buff.currtime), IPC_NOWAIT);
      }

      kill(runningProcess->realPid, SIGCONT);

      if (runningProcess->remainingtime == runningProcess->runningtime)
      {
        runningProcess->starttime = getClk();
        runningProcess->waitingtime = runningProcess->starttime - (runningProcess->arrivaltime);
      }
      else
      {
        int tempTime = getClk() - runningProcess->laststoptime;
        runningProcess->waitingtime += tempTime;
      }
      runningProcess->lastRunningClk = getClk();
    }
  }
  free(PQ);
}

void SRTNaddprocess()
{
  process *newprocess = initProcess();
  if (isEmpty(waitingQueue))
  {
    if (addProcess(memory_block, newprocess) == false)
    {
      printf("Process %d Added to waitingQueue\n", newprocess->id);
      normalQenqueue(waitingQueue, newprocess);
      return;
    }
    if (runningProcess != NULL)
    {
      runningProcess->remainingtime = runningProcess->remainingtime - (getClk() - runningProcess->lastRunningClk);
      runningProcess->lastRunningClk = getClk();
      SRTNenqueue(PQ, newprocess, newprocess->remainingtime);
      if (newprocess->remainingtime < runningProcess->remainingtime)
      {

        // printf("\nSWITCH!!!\n\n");

        ///////////////////////////// for message queue method //////////////////////////////

        proc_buff.currtime = getClk();
        proc_buff.mtype = runningProcess->realPid;

        // printf("Proc_buff data : currtime = %d -- mtype = %ld\n", proc_buff.currtime, proc_buff.mtype);

        msgsnd(proc_msgq_id, &proc_buff, sizeof(proc_buff.currtime), IPC_NOWAIT);

        kill(runningProcess->realPid, SIGTSTP);

        runningProcess->laststoptime = getClk();
        runningProcess = newprocess;

        kill(runningProcess->realPid, SIGCONT);
        runningProcess->lastRunningClk = getClk();
        if (runningProcess->remainingtime == runningProcess->runningtime)
        {
          runningProcess->starttime = getClk();
          runningProcess->waitingtime = runningProcess->starttime - (runningProcess->arrivaltime);
        }
        else
        {
          int tempTime = getClk() - runningProcess->laststoptime;
          runningProcess->waitingtime += tempTime;
        }
      }
    }
    else
    {
      SRTNenqueue(PQ, newprocess, newprocess->remainingtime);
    }
  }
  else
  {
    normalQenqueue(waitingQueue, newprocess);
  }
}

void HPF()
{
  PQ = createPriorityQueue();
  int prev = getClk();
  while (processCount > 0)
  {

    if (getClk() > prev && runningProcess)
    {
      prev = getClk();
      runningProcess->remainingtime = runningProcess->remainingtime - 1;
    }

    sch_rec_val = msgrcv(sch_msgq_id, &SCH_message, sizeof(SCH_message.arrivedProcess), getpid(), !IPC_NOWAIT);
    if (sch_rec_val != -1)
    {
      HPFaddprocess();
    }
    if (!PQisEmpty(PQ) && runningProcess == NULL)
    {
      runningProcess = PQpeek(PQ);
      runningProcess->starttime = getClk();
      runningProcess->waitingtime = runningProcess->starttime - (runningProcess->arrivaltime);
      runningProcess->lastRunningClk = getClk();
      kill(runningProcess->realPid, SIGCONT);
    }
  }
  free(PQ);
}

void HPFaddprocess()
{
  process *newprocess = initProcess();
  if (isEmpty(waitingQueue))
  {
    if (addProcess(memory_block, newprocess) == false)
    {
      printf("Process %d Added to waitingQueue\n", newprocess->id);
      normalQenqueue(waitingQueue, newprocess);
      return;
    }
    HPFenqueue(PQ, newprocess, newprocess->priority);
    if (PQpeek(PQ) == newprocess && runningProcess && runningProcess->remainingtime == runningProcess->runningtime) // change lw el running de lesa bad2a wl wesel priority a3la mnha
    {
      kill(newprocess->realPid, SIGCONT);
      kill(runningProcess->realPid, SIGTSTP);
      runningProcess = newprocess;
    }
  }
  else
  {
    normalQenqueue(waitingQueue, newprocess);
  }
}

int forkNewProcess(char *runnungtime)
{
  int id = fork();
  if (id == -1)
  {
    perror("error in fork");
    exit(-1);
  }
  else if (id == 0)
  {
    char quan[2];
    sprintf(quan, "%d", quantum);
    if (execl("./process.out", "process.out", runnungtime, quan, NULL) == -1)
    {
      perror("execl: ");
      exit(1);
    }
  }

  kill(id, SIGTSTP); // stop the forked (except if the ready queue is empty) process untill its turn

  return id;
}

///////////////////////////////////////////

void getAlgorithm()
{
  switch (algorithm)
  {
  case 1:
    printf("You are in HPF mode\n");
    HPF();
    break;
  case 2:
    printf("You are in SRTN mode\n");
    SRTN();
    break;
  case 3:
    printf("You are in RR mode\n");
    RRScheduler(quantum);
    break;
  }
}

///////////////////////////////////////////

void connectWithGenerator()
{
  sch_key_id = ftok("keyfile", 65);
  sch_msgq_id = msgget(sch_key_id, 0666 | IPC_CREAT);
  if (sch_msgq_id == -1)
  {
    perror("Error in create");
    exit(-1);
  }
}

////////////////////////////////////////////

void processReceivedSignal(int signum)
{
  // printf("process sent a signal\n");
  received = 1;
  signal(SIGUSR2, processReceivedSignal);
}

////////////////////////////////////////////

void sigtermhandler(int signum)
{
  if (Q)
    free(Q);
  if (PQ)
    free(PQ);
  kill(getpgrp(), SIGKILL);
  signal(SIGTERM, sigtermhandler);
}

void finishedPhandler(int signum)
{
  if (algorithm == 2)
  {
    process *finishedprocess = NULL;
    runningProcess = NULL;
    finishedprocess = PQdequeue(PQ);
    printf("Process ID = %d Fininshed at time = %d\n", finishedprocess->id, getClk());
    int memFlag = 0;
    updateMemory(memory_block, finishedprocess, &memFlag);

        while (!isEmpty(waitingQueue))
        {
         process *new = peek(waitingQueue);
          if (addProcess(memory_block, new) == false)
         {
          break;
         }
        dequeue(waitingQueue);
        SRTNenqueue(PQ, new, new->runningtime);
        }

    processCount--;
  }
  else if (algorithm == 1)
  {
    process *finishedprocess = runningProcess;
    printf("Process ID = %d Fininshed at time = %d\n", finishedprocess->id, getClk());
    // Remove the finished process from the queue
    PQremove(PQ, finishedprocess);
    free(finishedprocess);
    runningProcess = NULL;
    int memFlag = 0;
    updateMemory(memory_block, finishedprocess, &memFlag);

        while (!isEmpty(waitingQueue))
        {
         process *new = peek(waitingQueue);
          if (addProcess(memory_block, new) == false)
         {
          break;
         }
        dequeue(waitingQueue);
        HPFenqueue(PQ, new, new->priority);
        }
    processCount--;
  }
  else
  {
    // when a process finishes it should notify the scheduler on termination, the scheduler does NOT terminate the process.
    // the scheduler should remove the process from the queue and free its memory
    flag = 0;
  }
  signal(SIGUSR1, finishedPhandler);
}

///////////////////////////////////////////

void RRScheduler(int quantum)
{
  Q = createQueue();
  printf("Queue created with quantum = %d\n", quantum);

  while (true)
  {
    if (!isEmpty(Q))
    {
      runningProcess = dequeue(Q);

      int remainingtime = runningProcess->remainingtime;
      int runtime;

      if (quantum < remainingtime)
        runtime = quantum;
      else
        runtime = remainingtime;
      if (runningProcess->remainingtime == runningProcess->runningtime)
      {
        runningProcess->starttime = getClk();
        runningProcess->waitingtime = runningProcess->starttime - (runningProcess->arrivaltime);
      }
      else
      {
        int tempTime = getClk() - runningProcess->laststoptime;
        runningProcess->waitingtime += tempTime;
      }

      int curr, prev;
      curr = prev = getClk();
      int counter = 0;
      received = 1;

      printf("Process %c started at time = %d\n", ('A' + (runningProcess->id - 1)), curr);

      kill(runningProcess->realPid, SIGCONT);
      runningProcess->remainingtime -= runtime;
      received = 0;
      // Wait till you get a signal from the process that it received your signal
      while (!received){
         while ((msgrcv(sch_msgq_id, &SCH_message, sizeof(SCH_message.arrivedProcess), getpid(), IPC_NOWAIT)) != -1)
      {
        printf("Received\n");
        process *newprocess = initProcess();
        if (addProcess(memory_block, newprocess) == false){
          printf("Process %d Added to waitingQueue\n", newprocess->id);
          normalQenqueue(waitingQueue, newprocess);
        }else{
          normalQenqueue(Q, newprocess);
          }
      }
      };
      
    }

    // This loop checks for the incoming processes, if there is no incoming processes, it will break and continue running the current process
    if (processCount > 0)
    {
      usleep(1000);
      while ((msgrcv(sch_msgq_id, &SCH_message, sizeof(SCH_message.arrivedProcess), getpid(), IPC_NOWAIT)) != -1)
      {
        printf("Received\n");
        process *newprocess = initProcess();
        if (addProcess(memory_block, newprocess) == false){
          printf("Process %d Added to waitingQueue\n", newprocess->id);
          normalQenqueue(waitingQueue, newprocess);
        }else{
          normalQenqueue(Q, newprocess);
          }
      }
    }

    if (runningProcess)
    {
      // when a process fnishes it should notify the scheduler on termination, the scheduler does NOT terminate the process.
      if (flag)
      {
        // printf("%d Stopped\n", runningProcess->realPid);
        kill(runningProcess->realPid, SIGSTOP);

        runningProcess->laststoptime = getClk();
        normalQenqueue(Q, runningProcess);
      }
      else
      {
        int memFlag = 0;
        updateMemory(memory_block, runningProcess, &memFlag);
        while (!isEmpty(waitingQueue))
        {
         process *new = peek(waitingQueue);
          if (addProcess(memory_block, new) == false)
         {
          break;
         }
            dequeue(waitingQueue);
            normalQenqueue(Q, new);
        }
        printf("%c finished at time = %d\n", ('A' + (runningProcess->id - 1) ), getClk());
        free(runningProcess);
        processCount--;
        flag = 1;
        runningProcess = NULL;
        // printf("Done cleaning\n");
      }
    }

    if (processCount == 0)
      break;
  }

  free(Q);
}

///////////////////////////////////////////

process *initProcess()
{
  process *newprocess = createProcess(SCH_message.arrivedProcess.id, SCH_message.arrivedProcess.priority,
                                      SCH_message.arrivedProcess.arrivaltime, SCH_message.arrivedProcess.runningtime, SCH_message.arrivedProcess.memorySize);

  char runnungtimearg[20]; // a string containing the raunnumg time to be sent as argument to the forked process
  sprintf(runnungtimearg, "%d", newprocess->runningtime);

  int pid = forkNewProcess(runnungtimearg); // create a real process
  newprocess->realPid = pid;                // set the real id of the forked process
  return newprocess;
}