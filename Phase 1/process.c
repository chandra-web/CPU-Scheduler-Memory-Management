#include "Clock/headers.h"


int time_msgq_id;

struct sch_proc_buff time_buff;

/* Modify this file as needed*/
int remainingtime;

int prev, curr, counter = 0;


// int last_run;


void SIGCONThandler(int signum);
void SIGTSTPhandler(int signum);
// void srtnconthandler(int signum);


int main(int agrc, char *argv[])
{
  initClk();
  int runningtime = atoi(argv[1]);
  int quantum     = atoi(argv[2]);

  time_msgq_id = msgget(4000, 0666 | IPC_CREAT);
  if (time_msgq_id == -1)
  {
        perror("Error in create");
        exit(-1);
  }
 // else printf("PROCESS -- Message queue id = %d\n", time_msgq_id);

  signal(SIGCONT, SIGCONThandler);
  signal(SIGTSTP, SIGTSTPhandler);

  // signal(SIGCONT, srtnconthandler);
  
   
  remainingtime = runningtime;
  // printf("PID = %d initialized.\n", getpid());
  // printf("Starting process with ID = %d - Remaining time = %d - Started at time = %d\n", getpid(), remainingtime, getClk());

  prev = getClk();

  // last_run = getClk();
  // while(remainingtime > 0)
  // {
  //   if(getClk() > last_run )
  //   {
  //     last_run = getClk();
  //     remainingtime--;
  //     printf("PID = %d -- remaining time = %d\n", getpid(), remainingtime);
  //   }
  // }


  while (remainingtime > 0)
  {
    curr = getClk();
    if (curr != prev)
    {
      if (curr > prev)
      {
      //  printf("curr = %d - prev = %d\n", curr, prev);
        remainingtime--;
        //    printf("PID = %d - Remaining time = %d\n", getpid(), remainingtime);
        counter++; 
        if (counter == quantum && remainingtime > 0)
        {
          kill(getppid(), SIGUSR2);
          counter = 0;
        }
      }
      prev = getClk();
    }
  }
  // printf("Process with pid = %d - Finish time = %d\n",getpid(), prev);
  kill(getppid(), SIGUSR1);
  kill(getppid(), SIGUSR2);
  destroyClk(false);
  raise(SIGTERM);

  return 0;
}

void SIGCONThandler(int signum)
{
  // printf("ID = %d\n", getpid());
  prev = curr = getClk();

  if (msgrcv(time_msgq_id, &time_buff, sizeof(time_buff.currtime), getpid(), IPC_NOWAIT) != -1)  // for srtn
  {
    prev = time_buff.currtime;
    printf("PID = %d -- last_run = %d\n", getpid(), prev);
  }

}

void SIGTSTPhandler(int signum)
{

  if (msgrcv(time_msgq_id, &time_buff, sizeof(time_buff.currtime), getpid(), IPC_NOWAIT) != -1)
  {
    if (time_buff.currtime > prev)
    {
      remainingtime--;
    }
    printf("PID = %d -- last_stop = %d -- remaining time = %d\n", getpid(), time_buff.currtime, remainingtime);
  }

  raise(SIGSTOP);
}


// void srtnconthandler(int signum)
// {
//  if (msgrcv(time_msgq_id, &time_buff, sizeof(time_buff.currtime), getpid(), !IPC_NOWAIT) != -1)
//  {
//     last_run = time_buff.currtime;
//     printf("PID = %d -- last_run = %d\n", getpid(), last_run);
//  }
// }