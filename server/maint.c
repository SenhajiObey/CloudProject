#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#include "../utils_v10.h"
#include "prog.h"

#define SEM_KEY 248
#define KEY 369
#define PERM 0666
#define TABSIZE 1000



//******************************************************************************
//MAIN FUNCTION
//******************************************************************************
int main (int argc, char *argv[]) {
  
  const char *size = argv[1];
    int ia = *size - '0'; 

  struct Programme* p;
  //PREMIER CAS -> INITIALISE SHARED MEMORY + SEMAPHORE  
  if(ia==1){
      int sized = sizeof(p);
      printf("size = %d\n",sized);
      //SEMAPHORE
      int sem_id = sem_create(SEM_KEY, 1, PERM, 1);
      printf("Semaphore Created sem_id : %d \n",sem_id); 
      //SHARED MEMORY
      //TODO -> Change sizeof(int) par 50*sizeof(Prog)
      int shm_id = sshmget(KEY,(sizeof(p)*TABSIZE), IPC_CREAT | IPC_EXCL | PERM);
      int* z = sshmat(shm_id);
      printf("Shared Memory created ssh_id : %d at adress %d: \n",shm_id,*z);
      

      EXIT_SUCCESS;
  }
  //SECOND CAS -> DETRUIT LES RESSOURCES COMPLETES
  if(ia==2){
      //Destroy shared memory
      //Change sizeof(int) par 50*sizeof(Prog)
      int shm_id = sshmget(KEY, 0, 0);
      sshmdelete(shm_id);
      printf("Shared memory destroyed at : %d\n",shm_id);
      //Destroy semaphore
      int sem_id = sem_get(SEM_KEY,1);
      sem_delete(sem_id);
      printf("Semaphore with the adress : %d is destroyed\n",sem_id);

  }

  //TROISIEME CAS -> RESERVE LA MEMOIRE POUR UN TEMPS DONNEES
  if(ia==3){
      
      long int sleepTime = strtol(argv[2], NULL, 10);
      //int sleepTime = *size - '0';      
      int sem_id = sem_get(SEM_KEY,1);
      sem_down0(sem_id);
      sleep(sleepTime);
      sem_up0(sem_id);
      printf("Fin reservation\n");

  }

}


