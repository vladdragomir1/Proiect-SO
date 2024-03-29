/*Se primeste un folder ./program folder
Sa se faca un snapshot, se mai ruleaza inca o data sa se prinda ca s-a modificat*/

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>

DIR *deschideDirector(const char* name) {
    DIR *director = deschideDirector(name);
    if(director == NULL) {
        perror("Eroare deschiderea directorului");
        exit(1);
    }
    return director;
}

//void snapshot

int main(int argc, char **argv) {
    if(argc != 2)
    {
        printf("Nu sunt destule argumente\n");
        exit(-1);
    }

    DIR* dir = deschideDirector(argv[1]);

    close(dir);
    return 0;
}