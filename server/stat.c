#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include "../utils_v10.h"
#include "prog.h"

#define SEM_KEY 248
#define SHM_KEY 369
#define PERM 0666

int main(int argc, char *argv[]) {

    // Défini l'id du programme
    int idProgramme = *argv[1];

    // Obtention du sémaphore
    int sem_id = sem_get(SEM_KEY, 1);

    // Obtention de la mémoire partagée
    int shm_id = shmget(SHM_KEY, 0, 0);
    TabProgramme* tab = sshmat(shm_id);
    
    sem_down0(sem_id);
    printf("1. Numéro de programme : %i", (tab->tab)[idProgramme].id);
    printf("2. Nom du fichier source : %s", (tab->tab)[idProgramme].nom);
    printf("3. Erreur lors de compilation : %d", (tab->tab)[idProgramme].isErreur);
    printf("4. Nombre d'exécutions : %i", (tab->tab)[idProgramme].nbExecutions);
    printf("5. Temps d'exécution (en msec) : %i", (tab->tab)[idProgramme].tpsExecution);
    sem_up0(sem_id);

    sshmdt(tab);

    sshmdelete(shm_id);
}