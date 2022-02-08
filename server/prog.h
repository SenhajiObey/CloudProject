#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    int id;
    char* nom;
    bool isErreur;
    int nbExecutions;
    int tpsExecution;
} Programme;

typedef struct {
    Programme tab[50];
    int size;
} TabProgramme;
