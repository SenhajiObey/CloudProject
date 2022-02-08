#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/time.h>

#include "../utils_v10.h"
#include "prog.h"

#define SEM_KEY 248
#define SHM_KEY 369
#define PERM 0666

#define MAX_SIZE 255
#define MAX_BUF 255

volatile sig_atomic_t end = 0;
struct sockaddr_in addr;

long now();
int initSocketServer(int);
void endServerHandler(int);
int testCompilation(Programme);
void compil(void *);
void exec(void *);
int ajoutProgramme(Programme);
void exec_handler(void *);

// Variables globales
int shm_id, sem_id;
TabProgramme *tab;

long now()
{
    struct timeval tv;
    int res = gettimeofday(&tv, NULL);
    checkNeg(res, "Error gettimeofday");

    return tv.tv_sec * 1000000 + tv.tv_usec;
}

int initSocketServer(int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // setsockopt -> to avoid Address Already in Use
    int option = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int));

    // prepare sockaddr to bind
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    // listen port
    addr.sin_port = htons(port);
    // listen on all server interfaces
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));

    return sockfd;
}

void endServerHandler(int sig)
{
    end = 1;
}

int testCompilation(Programme prog)
{
    // Initialisation variables
    int ret;

    // Définition du path du text de l'erreur
    char pathCompil[MAX_SIZE];
    strcpy(pathCompil, prog.nom);
    pathCompil[strlen(pathCompil) - 2] = '\0';
    strcat(pathCompil, "_res_compile.txt");

    int fdCompil = sopen(pathCompil, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    int stderr_copy = dup(2); // fd = 2 -> flux d'erreur & dup pointe au même endroit
    checkNeg(stderr_copy, "ERROR dup");
    ret = dup2(fdCompil, 2);
    checkNeg(ret, "ERROR dup2");
    fork_and_run1(&compil, &prog.nom);

    // Définition du statut
    int status;
    swait(&status);
    ret = dup2(stderr_copy, 2);
    checkNeg(ret, "ERROR dup");
    sclose(stderr_copy);
    sclose(fdCompil);
    return status;
}

void compil(void *path)
{
    char *pathProg = path;
    char pathFolder[MAX_SIZE];
    strcpy(pathFolder, pathProg);
    pathFolder[strlen(pathProg) - 2] = '\0';

    execl("/usr/bin/gcc", "gcc", "-o", pathFolder, pathProg, NULL);
    perror("Error execl 1");
}

void exec(void *path)
{
    char *pathProg = path;
    char pathFolder[MAX_SIZE];
    strcpy(pathFolder, pathProg);
    pathFolder[strlen(pathProg) - 2] = '\0';

    execl(pathFolder, pathProg, NULL);
    perror("Error exec 2");
}

int ajoutProgramme(Programme prog)
{
    sem_down0(sem_id);
    prog.id = tab->size;
    (tab->tab[tab->size]) = prog;
    tab->size = tab->size + 1;
    sem_up0(sem_id);
    return prog.id;
}

void exec_handler(void *argv)
{

    // Variable pour le socket
    int *sockfd = argv;
    int newsockfd = *sockfd;
    printf("exec_handler_newsockfd %d", newsockfd);
    int req, ret;
    Programme prog;

    // Read le socket
    sread(newsockfd, &req, sizeof(int));
    /*2.a.i*/
    printf("Jusqu'ici ça va %d", req);
    // AJOUT ET MODIFICATION
    if (req != -2)
    {
        char path[MAX_SIZE] = "programmes/";
        char buf[MAX_BUF];
        int fd;
        int readChar;

        // AJOUT
        if (req == -1)
        {
            int sizeName;
            char fileName[MAX_SIZE];
            prog.nbExecutions = 0;
            prog.tpsExecution = 0;
            int id = ajoutProgramme(prog);
            prog.id = id;

            // Concaténation de path et nom
            /*2.a.ii*/
            sread(newsockfd, &sizeName, sizeof(int));
            printf("sizename : %d", sizeName);
            /*2.a.iii*/
            sread(newsockfd, &fileName, sizeName * sizeof(char));
            printf("file name :%s", fileName);
            strcat(path, fileName);

            // Ouverture du fichier
            fd = sopen(path, O_WRONLY | O_TRUNC | O_CREAT, 0644);
            while ((readChar = sread(newsockfd, buf, MAX_BUF * sizeof(char))) != 0)
            {
                // Ecriture
                swrite(fd, buf, readChar * sizeof(char));
            }
            // Fermeture du fichier
            sclose(fd);
            strcpy(prog.nom, path);

            // Renvoie id
            swrite(newsockfd, &id, sizeof(int));

            // Test compilation
            int status = testCompilation(prog);
            swrite(newsockfd, &status, sizeof(int));

            if (WIFEXITED(status))
            {
                // Défini erreur false
                prog.isErreur = false;
            }
            else
            {
                // Défini erreur true
                prog.isErreur = true;
                // Défini message enregistré dans txt
                char bufErr[MAX_BUF];
                path[strlen(path) - 2] = '\0';
                strcat(path, "_res_compile.txt");
                int fdErr = sopen(path, O_RDONLY | O_CREAT, 0666);
                while ((readChar = sread(fdErr, bufErr, MAX_BUF * sizeof(char))) != 0)
                {
                    swrite(newsockfd, bufErr, readChar * sizeof(char));
                }
                sclose(fdErr);
            }

            // Set le programme
            sem_down0(sem_id);
            (tab->tab)[id] = prog;
            sem_up0(sem_id);

            // MODIFICATION
        }
        else
        {
            prog = (tab->tab)[req];
            int id = prog.id;
            strcat(path, prog.nom);

            // Ouverture du fichier
            fd = sopen(path, O_WRONLY | O_TRUNC | O_CREAT, 0644);
            while ((readChar = sread(newsockfd, buf, MAX_BUF * sizeof(char))) != 0)
            {
                // Ecriture
                swrite(fd, buf, readChar * sizeof(char));
            }
            // Fermeture du fichier
            sclose(fd);
            strcpy(prog.nom, path);
            swrite(newsockfd, &id, sizeof(int));
        }

        // EXECUTION
    }
    else if(req == -2)
    {

        // Get l'id du programme
        int id;
        sread(newsockfd, &id, sizeof(int));
        swrite(newsockfd, &id, sizeof(int));
        printf("id du programme %d", id);
        // Return -2 si le programme n'existe pas
        if (id >= tab->size || id < 0)
        {
            int req = -2;
            swrite(newsockfd, &req, sizeof(int));
        }
        else
        {

            // Get le programme
            prog = (tab->tab)[id];

            // Test compilation
            int status = testCompilation(prog);

            // Si le fils se fini normalement
            if (WIFEXITED(status))
            {
                prog.isErreur = false;

                char pathRes[MAX_SIZE];
                strcpy(pathRes, prog.nom);
                pathRes[strlen(pathRes) - 2] = '\0';
                strcat(pathRes, "_res_execution.txt");

                int fdExec = sopen(pathRes, O_CREAT | O_WRONLY | O_TRUNC, 0666);

                int stdout = dup(1);
                checkNeg(stdout, "ERROR dup");

                ret = dup2(fdExec, 1);
                checkNeg(ret, "ERROR dup2");

                // Exécution
                long t1 = now();
                fork_and_run1(&exec, &prog.nom);
                swait(&status);
                ret = dup2(stdout, 1);
                checkNeg(ret, "ERROR dup");
                sclose(stdout);
                sclose(fdExec);
                long t2 = now();

                // Définition du temps d'exécution & nombre d'executions
                long execTime = t2 - t1;
                prog.nbExecutions += 1;
                prog.tpsExecution += execTime;

                int progStatus = WEXITSTATUS(status);
                if (WIFEXITED(status) != 0)
                {
                    // Return 1 si le programme se finit normalement
                    int req = 1;
                    swrite(newsockfd, &req, sizeof(int));
                    // Envoie l'entier indiquant le temps d'exécution
                    swrite(newsockfd, &execTime, sizeof(long));
                    // Envoie l'entier le code de retour
                    swrite(newsockfd, &progStatus, sizeof(int));

                    // Envoie la sortie standard du programme exécuté à distance
                    int fd = sopen(pathRes, O_RDONLY | O_CREAT, 0666);
                    char buffer[MAX_SIZE];
                    int readChar;
                    while ((readChar = sread(fd, buffer, MAX_SIZE * sizeof(char))) != 0)
                    {
                        swrite(newsockfd, &buffer, readChar * sizeof(char));
                    }
                    sclose(fd);
                }
                else
                {
                    // Return 0 si le programme ne se finit pas normalement
                    int req = 0;
                    swrite(newsockfd, &req, sizeof(int));
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{

    // Socket
    int sockfd, newsockfd;

    // Obtention de la mémoire partagée
    shm_id = sshmget(SHM_KEY, 0, 0);
    tab = sshmat(shm_id);

    // Obtention de la sémaphore
    sem_id = sem_get(SEM_KEY, 1);

    // Initialise le serveur
    sockfd = initSocketServer(atoi(argv[1]));
    printf("Le serveur tourne sur le port %i\n", atoi(argv[1]));

    // simultaneous client max -> 5
    listen(sockfd, 5);

    // Signal Ctrl+C
    ssigaction(SIGINT, endServerHandler);

    while (end == 0)
    {

        // accept client connection
        newsockfd = accept(sockfd, NULL, NULL);
        printf("New connection %d", newsockfd);
        if (newsockfd > 0)
        {
            fork_and_run1(&exec_handler, &newsockfd);
            // close connection client
            sclose(newsockfd);
        }
    }
    sshmdt(tab);
    return 0;
}
