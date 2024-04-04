/*Se primeste un folder ./program folder
Sa se faca un snapshot, se mai ruleaza inca o data sa se prinda ca s-a modificat*/

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

// Functie pentru a deschide un director si a verifica erorile
DIR* deschideDirector(const char* name) {
    DIR *director = opendir(name);
    if(director == NULL) {
        perror("Eroare deschiderea directorului");
        exit(1);
    }
    return director;
}

// Salveaza starea curenta a directorului in fisierul snapshot.txt
void snapshot(const char* directoryPath) {
    DIR* dir = deschideDirector(directoryPath);
    struct dirent* entry;

    FILE* file = fopen("snapshot.txt", "w");
    if(file == NULL) {
        perror("Eroare la deschiderea fisierului snapshot");
        exit(1);
    }

    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            fprintf(file, "%s\n", entry->d_name);
        }
    }

    fclose(file);
    closedir(dir);
}

// Detecteaza schimbarile fata de ultimul snapshot
void detectChanges(const char* directoryPath) {
    DIR* dir = deschideDirector(directoryPath);
    struct dirent* entry;

    FILE* file = fopen("snapshot.txt", "r");
    if(file == NULL) {
        perror("Eroare la deschiderea fisierului snapshot pentru comparatie");
        exit(1);
    }

    char buffer[256];
    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            int found = 0;
            rewind(file); // Mergi la inceputul fisierului pentru fiecare fisier din director
            while(fgets(buffer, sizeof(buffer), file)) {
                buffer[strcspn(buffer, "\n")] = 0; // Elimina newline-ul
                if(strcmp(entry->d_name, buffer) == 0) {
                    found = 1;
                    break;
                }
            }
            if(!found) {
                printf("Fisier nou sau modificat: %s\n", entry->d_name);
            }
        }
    }

    fclose(file);
    closedir(dir);
}

int main(int argc, char **argv) {
    if(argc != 3) {
        printf("Utilizare: %s <cale director> <snapshot/detect>\n", argv[0]);
        exit(-1);
    }

    if(strcmp(argv[2], "snapshot") == 0) {
        snapshot(argv[1]);
    } else if(strcmp(argv[2], "detect") == 0) {
        detectChanges(argv[1]);
    } else {
        printf("Comanda necunoscuta. Folositi 'snapshot' sau 'detect'.\n");
        exit(-1);
    }

    return 0;
}
