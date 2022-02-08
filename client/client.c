#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <fcntl.h>

#include "../utils_v10.h"

#define MAX_BUFFER_SIZE 150
#define MAX_SIZE_EXECUTION_RECURRENTE 100
#define MAX_BUFFER_SIZE_PIPE 10

void envoyerProgServeur(char *commande);
void remplacerProgServeur(char *commande);
void reponseServeur(int sockfd);
void addExecutionRecurrente(int numeroProgramme);
void demandeExeServeur(int numProgramme);
void reponseServeurExecution(int sockfd);
void minuterie(void *delay);
void execution();

/*variable*/
int pipefd[2];
char *adr;
int port;

/************ TERMINAL **********/
int main(int argc, char **argv)
{

    if (argc != 4)
    {
        printf("Nombre d'argument invalide \n");
        exit(2);
    }

    /*Place argument du programme dans des variables*/
    adr = argv[1];
    port = atoi(argv[2]);
    int delay = atoi(argv[3]);
    /*Création du pipe*/

    int ret = spipe(pipefd);
    checkNeg(ret, "pipe error");

    /*Création des enfants et passage du pipe+socket aux enfants */

    int ppidMinute = fork_and_run1(minuterie, &delay);

    int ppidExeRecurrente = fork_and_run0(execution);

    /*On est dans le main*/

    /*Ferme le pipe de lecture*/
    int retour = close(pipefd[0]);
    checkNeg(retour, "close error");

    char cmd[MAX_BUFFER_SIZE];

    while (true)
    {
        printf(" :: ");
        sread(0, cmd, MAX_BUFFER_SIZE);

        if (cmd[0] == '+')
        {
            char *hello = "J'envoie un programme au serveur\n";
            size_t sz = strlen(hello);
            nwrite(0, hello, sz);
            envoyerProgServeur(cmd);
        }
        else if (cmd[0] == '-')
        {
            char *hello = "Je remplace le fichier C portant le num\n";
            size_t sz = strlen(hello);
            nwrite(0, hello, sz);
            remplacerProgServeur(cmd);
        }
        else if (cmd[0] == '*')
        {
            char *hello = "J'envoie le programme à faire de manière récurrente\n";
            size_t sz = strlen(hello);
            nwrite(0, hello, sz);
            int numProgramme = atoi(&cmd[2]);
            addExecutionRecurrente(numProgramme);
        }
        else if (cmd[0] == '@')
        {
            char *hello = "Je demande l'execution du programme \n";
            size_t sz = strlen(hello);
            nwrite(0, hello, sz);
            int numProgramme = atoi(&cmd[2]);
            demandeExeServeur(numProgramme);
        }
        else if (cmd[0] == 'q')
        {
            skill(ppidExeRecurrente, SIGQUIT);
            skill(ppidMinute, SIGQUIT);
            skill(getpid(),SIGQUIT);
            exit(0);
        }
    }
}

void remplacerProgServeur(char *commande)
{
    printf(" ENVOIE PROGRAMME SERVEUR REMPLACEMENT\n");

    int sockfd = ssocket();

    sconnect(adr, port, sockfd);
    int commandNumber = atoi(commande + 2);
    /*chemin du fichier*/
    int longueurCheminFichierSource = strlen(commande + 4);
    int longueurNomFichierSource = -1;
    for (size_t i = longueurCheminFichierSource; i >= 0; i--)
    {
        if (commande[i] == '/')
        {
            longueurCheminFichierSource = i + 1;
            break;
        }
    }
    /*envoie le command number -> 2.a.i*/
    nwrite(sockfd, &commandNumber, sizeof(int));
    /*envoie le nombre caractère nom fichier source -> 2.a.ii*/
    nwrite(sockfd, &longueurNomFichierSource, sizeof(int));
    /*envoie le nom du fichier source -> 2.a.iii*/
    if (longueurNomFichierSource == -1)
    {
        nwrite(sockfd, commande + 4, longueurCheminFichierSource * sizeof(char));
    }
    else
    {
        nwrite(sockfd, commande + longueurNomFichierSource, longueurCheminFichierSource * sizeof(char));
    }

    /*Ouverture du fichier en C + envoie sur sockfd -> 2.a.iv*/
    int fd = sopen(commande + 2, O_RDONLY | O_CREAT, 0666);
    char buffer[MAX_BUFFER_SIZE];
    int nbCharRead;
    while ((nbCharRead = sread(fd, buffer, MAX_BUFFER_SIZE * sizeof(char))) != 0)
    {
        nwrite(sockfd, &buffer, nbCharRead * sizeof(char));
        printf("%s",buffer);
    }
    /*fermeture du fichier C*/
    sclose(fd);
    /*Lecture de la réponse du serveur*/
    reponseServeur(sockfd);
    /*Fermeture une fois fini*/
    sclose(sockfd);
}


int geNomFichierFromChemin(char *commande,int longueurCheminFichierSource){
    int index = -1;
    for (size_t i = longueurCheminFichierSource; i >= 0; i--)
    {
       if(commande[i] == '/'){
           index = i+1;
           break;
       } 
    }
    if(index<=0){
        return longueurCheminFichierSource;
    }else{
        return index;
    }

    
}

void envoyerProgServeur(char *commande)
{
    commande[strlen(commande)-1] = '\0';
    printf(" ENVOIE PROGRAMME SERVEUR \n");
    int sockfd = ssocket();

    sconnect(adr, port, sockfd);
    int commandNumber = -1;
    /*chemin du fichier*/
    int longueurCheminFichierSource = strlen(commande + 2);
    int longueurNomFichierSource = geNomFichierFromChemin(commande, longueurCheminFichierSource);
    
    /*envoie le command number -> 2.a.i*/
    nwrite(sockfd, &commandNumber, sizeof(int));
    printf("valeur de commandNumber après envoie %d \n",commandNumber);
    /*envoie le nombre caractère nom fichier source -> 2.a.ii*/
    nwrite(sockfd, &longueurNomFichierSource, sizeof(int));
    printf("valeur de commandNumber après envoie %d\n",longueurNomFichierSource);
    /*envoie le nom du fichier source -> 2.a.iii*/
    
    if (longueurNomFichierSource == -1)
    {
        nwrite(sockfd, commande + 2, longueurCheminFichierSource * sizeof(char));
    }
    else
    {
        nwrite(sockfd, commande + 2, longueurCheminFichierSource * sizeof(char));
    }

    /*Ouverture du fichier en C + envoie sur sockfd -> 2.a.iv*/
    printf("Chemin du fichier : .%s.\n",commande+2);
    int fd = sopen(commande + 2, O_RDONLY | O_CREAT, 0644);
    
    char buffer[MAX_BUFFER_SIZE];
    int nbCharRead;
    while ((nbCharRead = sread(fd, buffer, MAX_BUFFER_SIZE * sizeof(char))) != 0)
    {
        printf("buffer :: %s",buffer);
        swrite(sockfd, buffer, nbCharRead * sizeof(char));
        printf("Je suis passé");
    }
    /*fermeture du fichier C*/
    sclose(fd);
    /*Lecture de la réponse du serveur*/
    printf("Réponse serveur :: ");
    reponseServeur(sockfd);
    /*Fermeture une fois fini*/
    sclose(sockfd);
}

void reponseServeur(int sockfd)
{
    int numeroProgramme;
    char buffer[MAX_BUFFER_SIZE];
    int status;
    int nbCharRead;
    sread(sockfd, &numeroProgramme, sizeof(int));
    sread(sockfd,&status,sizeof(int));
    printf("Le programme ajouté porte le numéro %d status compilation %d\n", numeroProgramme, status);
    while ((nbCharRead = sread(sockfd, buffer, MAX_BUFFER_SIZE * sizeof(char))) != 0)
    {
        printf("%s\n", buffer);
    }
}

void addExecutionRecurrente(int numeroProgramme)
{
    char buffer[MAX_BUFFER_SIZE_PIPE];
    buffer[0] = 'a';
    buffer[2] = (char) numeroProgramme;
    nwrite(pipefd[1], &buffer, MAX_BUFFER_SIZE_PIPE*sizeof(char));
}

void demandeExeServeur(int numProgramme)
{
    /* Demande Execution au serveur*/
    printf(" EXECUTION PROGRAMME SERVEUR \n");
    

    int sockfd = ssocket();

    int isConnected = sconnect(adr, port, sockfd);
    printf("is Connected %d", isConnected);
    int commandNumber = -2;
    printf("Envoie du code Number %d",commandNumber);
    int retour = swrite(sockfd, &commandNumber, sizeof(int));
    printf("valeur retour premier write %d",retour);
    printf("Le numéro de programme : %d",numProgramme);
    
    swrite(sockfd, &numProgramme, sizeof(int));
    reponseServeurExecution(sockfd);
    sclose(sockfd);
}

void reponseServeurExecution(int sockfd)
{
    int numeroAssocieProgramme;
    int etatProgramme;
    sread(sockfd, &numeroAssocieProgramme, sizeof(int));
    sread(sockfd, &etatProgramme, sizeof(int));
    printf("Execution du programme avec le numero : %d\n", numeroAssocieProgramme);
    if (etatProgramme == -2)
    {
        printf("Le programme n'existe pas");
        return;
    }
    else if (etatProgramme == -1)
    {
        printf("Le programme ne compile pas");
    }
    else if (etatProgramme == 0)
    {
        printf("Le programme ne s'est pas termine normalement");
    }
    else if (etatProgramme == 1)
    {
        printf("Le programme s'est termine normalement");
        /*3.b.iii*/
        long tempsExecution;
        sread(sockfd, &tempsExecution, sizeof(long));
        printf("Temps d'execution :: %ld ms\n", tempsExecution);
        /*3.b.iv*/
        int codeRetour;
        sread(sockfd, &codeRetour, sizeof(int));
        printf("Code de retour :: %d \n", codeRetour);
        /*3.b.v*/
        char buffer[MAX_BUFFER_SIZE];
        int nbCharRead;
        while ((nbCharRead = sread(sockfd, buffer, MAX_BUFFER_SIZE * sizeof(char))) != 0)
        {
            printf("%s\n", buffer);
        }
    }
    else
    {
        printf("Numero d'etat inconnu ");
    }
}

void minuterie(void *delay)
{   
    /*Cast de void à int*/
    int* duration = delay;
    int durationInt = *duration;
    /*Fermeture de la pipe en lecture*/
    sclose(pipefd[0]);
    while(true){
        sleep(durationInt);
        char buffer[MAX_BUFFER_SIZE_PIPE];
        /*b pour battement*/
        buffer[0] = 'b';
        nwrite(pipefd[1], &buffer, MAX_BUFFER_SIZE_PIPE*sizeof(char));
    }

}

void execution()
{
    /*Fermeture du pipe en écriture*/
    sclose(pipefd[1]);
    int tab[MAX_SIZE_EXECUTION_RECURRENTE];
    int tailleLogique = 0;
    while (true)
    {
        char buffer[MAX_BUFFER_SIZE];
        sread(pipefd[0],buffer,MAX_BUFFER_SIZE*sizeof(char));
        /*Si on recoit un a (pour add) de la part de l'ajout d'un programme récurrent (*) du main */
        if(buffer[0] == 'a'){
            tab[tailleLogique] = atoi(buffer+2);
            tailleLogique++;
        }else{
            /*On a recu un battement donc on execute toute la liste*/
            for (int i = 0; i < tailleLogique; i++)
            {
                demandeExeServeur(tab[i]);
            }
            
        }
    }
}
