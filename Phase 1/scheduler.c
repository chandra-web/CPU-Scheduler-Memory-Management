#include "DataStructures/PriorityQueue.h"
#include <sys/file.h>
#include <math.h>
#include <stdio.h>

PriorityQueue *PQ = NULL;
Queue *Q = NULL;
Queue *finishedQueue = NULL;

int TA;
float WTA;
float *allWTA;


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

FILE *schedulerLog, *schedulerPref;

process *runningProcess = NULL;

/////// FUNCTIONS ////////


void connectWithGenerator();
void getAlgorithm();

void finishedPhandler(int signum);
void sigtermhandler(int signum);

int forkNewProcess(char *runtime);
process *initProcess();


void processReceivedSignal(int signum);
void RRScheduler(int quantum);

void SRTN();
void SRTNaddprocess();

void HPF();
void HPFaddprocess();

void printProcessState();
void printProcessFinish();
void outputFile();

int main(int argc, char *argv[])
{
  initClk();

  signal(SIGTERM, sigtermhandler);        // to free the allocated memory
  signal(SIGUSR1, finishedPhandler);      // to recieve that a process has finished its execution
  signal(SIGUSR2, processReceivedSignal); // to check if a process received a continue signal
  // schedulerLog=fopen("scheduler.log", "a+");
  schedulerLog = fopen("scheduler.log", "w");
  // schedulerPref=fopen("scheduler.pref", "a+");
  schedulerPref = fopen("scheduler.pref", "w");
  algorithm = atoi(argv[1]);
  sprintf(salgo, "%d", algorithm);
  processCount = atoi(argv[2]);
  currentProcessCount = processCount;
  quantum = atoi(argv[3]);
  allWTA = malloc(currentProcessCount * sizeof(float));
  finishedQueue = createQueue();
  connectWithGenerator();

 proc_msgq_id = msgget(4000, 0666 | IPC_CREAT);
 if (proc_msgq_id == -1)
 {
      perror("Error in create");
      exit(-1);
 }
 else printf("Message queue id = %d\n", proc_msgq_id);

  getAlgorithm();

  // TODO implement the scheduler :)
  outputFile();
  destroyClk(true);
  fclose(schedulerLog);
  fclose(schedulerPref);
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

      if (runningProcess->runningtime != runningProcess->remainingtime)  // if it is the first run for the process, the handler is not attatched to the signal yet
      {
        proc_buff.currtime = getClk();
        proc_buff.mtype = runningProcess->realPid;

        printf("Proc_buff data : currtime = %d -- mtype = %ld\n", proc_buff.currtime, proc_buff.mtype);

        msgsnd(proc_msgq_id, &proc_buff, sizeof(proc_buff.currtime), IPC_NOWAIT);

      }

      kill(runningProcess->realPid, SIGCONT);

      if (runningProcess->remainingtime == runningProcess->runningtime)
      {
        runningProcess->starttime = getClk();
        runningProcess->waitingtime = runningProcess->starttime - (runningProcess->arrivaltime);
        fprintf(schedulerLog, "#At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), runningProcess->id, "started", runningProcess->arrivaltime, runningProcess->runningtime, runningProcess->remainingtime, runningProcess->waitingtime);
        fflush(schedulerLog);
      }
      else
      {
        int tempTime = getClk() - runningProcess->laststoptime;
        runningProcess->waitingtime += tempTime;
        fprintf(schedulerLog, "#At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), runningProcess->id, "resumed", runningProcess->arrivaltime, runningProcess->runningtime, runningProcess->remainingtime, runningProcess->waitingtime);
        fflush(schedulerLog);
      }
      runningProcess->lastRunningClk = getClk();
    }
  }
  free(PQ);
}

void SRTNaddprocess()
{
  process *newprocess = initProcess();
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


      fprintf(schedulerLog, "#At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), runningProcess->id, "stopped", runningProcess->arrivaltime, runningProcess->runningtime, runningProcess->remainingtime, runningProcess->waitingtime);
      fflush(schedulerLog);

      runningProcess->laststoptime = getClk();
      runningProcess = newprocess;


      kill(runningProcess->realPid, SIGCONT);
      runningProcess->lastRunningClk = getClk();
      if (runningProcess->remainingtime == runningProcess->runningtime)
      {
        runningProcess->starttime = getClk();
        runningProcess->waitingtime = runningProcess->starttime - (runningProcess->arrivaltime);

        fprintf(schedulerLog, "#At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), runningProcess->id, "started", runningProcess->arrivaltime, runningProcess->runningtime, runningProcess->remainingtime, runningProcess->waitingtime);
        fflush(schedulerLog);
      }
      else
      {
        int tempTime = getClk() - runningProcess->laststoptime;
        runningProcess->waitingtime += tempTime;
        fprintf(schedulerLog, "#At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), runningProcess->id, "resumed", runningProcess->arrivaltime, runningProcess->runningtime, runningProcess->remainingtime, runningProcess->waitingtime);

        fflush(schedulerLog);
      }
    }
  }
  else
  {
    SRTNenqueue(PQ, newprocess, newprocess->remainingtime);
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
      fprintf(schedulerLog, "#At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), runningProcess->id, "started", runningProcess->arrivaltime, runningProcess->runningtime, runningProcess->remainingtime, runningProcess->waitingtime);

      fflush(schedulerLog);
      runningProcess->lastRunningClk = getClk();
      kill(runningProcess->realPid, SIGCONT);
    }
  }
  free(PQ);
}

void HPFaddprocess()
{
  process *newprocess = initProcess();
  HPFenqueue(PQ, newprocess, newprocess->priority);
  if (PQpeek(PQ) == newprocess && runningProcess && runningProcess->remainingtime == runningProcess->runningtime) // change lw el running de lesa bad2a wl wesel priority a3la mnha
  {
    kill(newprocess->realPid, SIGCONT);
    kill(runningProcess->realPid, SIGTSTP);
    runningProcess = newprocess;
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
  free(Q);
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
    int TA = getClk() - finishedprocess->arrivaltime;
    float WTA = (float)TA / (float)finishedprocess->runningtime;
    totalWTA += WTA;
    allWTA[processCount - 1] = WTA;
    totalWaitingTime += finishedprocess->waitingtime;
    totalRT += finishedprocess->runningtime;
    fprintf(schedulerLog, "#At time %d process %d %s arr %d total %d remain %d wait %d TA %d WTA %.2f\n", getClk(), finishedprocess->id, "finished", finishedprocess->arrivaltime, finishedprocess->runningtime, 0, finishedprocess->waitingtime, TA, WTA);
    fflush(schedulerLog);

    processCount--;
  }
  else if (algorithm == 1)
  {
    process *finishedprocess = runningProcess;
    printf("Process ID = %d Fininshed at time = %d\n", finishedprocess->id, getClk());
    int TA = getClk() - finishedprocess->arrivaltime;
    float WTA = (float)TA / (float)finishedprocess->runningtime;
    fprintf(schedulerLog, "#At time %d process %d %s arr %d total %d remain %d wait %d TA %d WTA %.2f\n", getClk(), finishedprocess->id, "finished", finishedprocess->arrivaltime, finishedprocess->runningtime, 0, finishedprocess->waitingtime, TA, WTA);
    fflush(schedulerLog);

    allWTA[processCount - 1] = WTA;
    totalWTA += WTA;

    totalWaitingTime += finishedprocess->waitingtime;
    totalRT += finishedprocess->runningtime;
    // Remove the finished process from the queue
    PQremove(PQ, finishedprocess);
    free(finishedprocess);
    runningProcess = NULL;
    processCount--;
  }
  else
  {
    // when a process fnishes it should notify the scheduler on termination, the scheduler does NOT terminate the process.
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

        fprintf(schedulerLog, "#At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), runningProcess->id, "started", runningProcess->arrivaltime, runningProcess->runningtime, runningProcess->remainingtime, runningProcess->waitingtime);
        fflush(schedulerLog);
      }
      else
      {
        int tempTime = getClk() - runningProcess->laststoptime;
        runningProcess->waitingtime += tempTime;
        fprintf(schedulerLog, "#At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), runningProcess->id, "resumed", runningProcess->arrivaltime, runningProcess->runningtime, runningProcess->remainingtime, runningProcess->waitingtime);

        fflush(schedulerLog);
      }

      int curr, prev;
      curr = prev = getClk();
      int counter = 0;
      received = 1;

      printf("Process %c started at time = %d\n", ('A' + (runningProcess->id - 1) ), curr);

      kill(runningProcess->realPid, SIGCONT);
      runningProcess->remainingtime -= runtime;
      received = 0;

      // printf("Waiting\n");
      // Wait till you get a signal from the process that it received your signal
      while (!received);
      // printf("signaled\n");
      
    }

    // This loop checks for the incoming processes, if there is no incoming processes, it will break and continue running the current process
    if (processCount > 0)
    {
      usleep(1000);
      while ((msgrcv(sch_msgq_id, &SCH_message, sizeof(SCH_message.arrivedProcess), getpid(), IPC_NOWAIT)) != -1)
      {
        printf("Received\n");
        process *newprocess = initProcess();
        normalQenqueue(Q, newprocess);
      }
    }

    if (runningProcess)
    {
      // when a process fnishes it should notify the scheduler on termination, the scheduler does NOT terminate the process.
      if (flag)
      {
        // printf("%d Stopped\n", runningProcess->realPid);
        kill(runningProcess->realPid, SIGSTOP);
        fprintf(schedulerLog, "#At time %d process %d %s arr %d total %d remain %d wait %d\n", getClk(), runningProcess->id, "stopped", runningProcess->arrivaltime, runningProcess->runningtime - (runningProcess->remainingtime), runningProcess->remainingtime, runningProcess->waitingtime);
        fflush(schedulerLog);

        runningProcess->laststoptime = getClk();
        normalQenqueue(Q, runningProcess);
      }
      else
      {
        int TA = getClk() - runningProcess->arrivaltime;
        float WTA = (float)TA / (float)runningProcess->runningtime;
        fprintf(schedulerLog, "#At time %d process %d %s arr %d total %d remain %d wait %d TA %d WTA %.2f\n", getClk(), runningProcess->id, "finished", runningProcess->arrivaltime, runningProcess->runningtime, 0, runningProcess->waitingtime, TA, WTA);
        fflush(schedulerLog);

        allWTA[processCount - 1] = WTA;
        totalWTA += WTA;
        totalWaitingTime += runningProcess->waitingtime;
        totalRT += runningProcess->runningtime;

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
                                      SCH_message.arrivedProcess.arrivaltime, SCH_message.arrivedProcess.runningtime);

  char runnungtimearg[20]; // a string containing the raunnumg time to be sent as argument to the forked process
  sprintf(runnungtimearg, "%d", newprocess->runningtime);

  // char arrivaltime[20]; // same for arrival time (msh 3aref hn7tagha wla la)
  // sprintf(arrivaltime, "%d", newprocess->arrivaltime);

  int pid = forkNewProcess(runnungtimearg); // create a real process
  newprocess->realPid = pid;   // set the real id of the forked process
  return newprocess;
}

void outputFile()
{

  float avgWTA = (float)totalWTA / currentProcessCount;
  float avgWaitingTime = (float)totalWaitingTime / currentProcessCount;
  float CPUutilization = (float)totalRT / getClk() * 100;
  float currentTime = 0, StandardDeviation = 0;
  for (int i = 0; i < currentProcessCount; i++)
  {
    StandardDeviation += pow(allWTA[i] - avgWTA, 2);
  }
  StandardDeviation = sqrt(StandardDeviation / currentProcessCount);

  fprintf(schedulerPref, "CPU Utilization = %.2f%% \nAvg WTA = %.2f\nAvg WT = %.2f\nStd WTA = %.2f\n",
          CPUutilization, avgWTA, avgWaitingTime, StandardDeviation);
  fflush(schedulerPref);

  printf("Output generated successfully\n");
}