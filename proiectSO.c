#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>

void createSnapshot(const char *directoryPath) {
    DIR *dir;
    struct dirent *entry;
    struct stat fileInfo;
    char filePath[1024];
    time_t t;
    struct tm *tmp;
    char lastModifiedTime[50];
 
    dir = opendir(directoryPath);
    if(dir == NULL) {
        perror("Unable to open directory");
        exit(EXIT_FAILURE);
    }

    snprintf(filePath, sizeof(filePath), "%s/Snapshot.txt", directoryPath);
    FILE *snapshotFile = fopen(filePath, "w"); // Change open to fopen and use "w" to write to the file.

    if(snapshotFile == NULL) {
        perror("Unable to create snapshot file");
        exit(EXIT_FAILURE);
    }

    while((entry = readdir(dir)) != NULL) {
        snprintf(filePath, sizeof(filePath), "%s/%s", directoryPath, entry->d_name);
        if(stat(filePath, &fileInfo) == 0) {
            t = fileInfo.st_mtime;
            tmp = localtime(&t);
            strftime(lastModifiedTime, sizeof(lastModifiedTime), "%Y-%m-%d %H:%M:%S", tmp);
            if(S_ISDIR(fileInfo.st_mode)) {
                write(snapshotFile, "Directory: %s, Last Modified: %s\n", entry->d_name, lastModifiedTime);
            } else {
                fprintf(snapshotFile, "File: %s, Size: %ld, Last Modified: %s\n", entry->d_name, fileInfo.st_size, lastModifiedTime);
            }
        }
    }
    fclose(snapshotFile);
    closedir(dir);
}

int main(int argc, char *argv[]) {

    if(argc > 10) {
        fprintf(stderr, "Usage: %s [directory_path]...\n", argv[0]);
        return EXIT_FAILURE;
    }

    pid_t pid;
    for(int i = 1; i < argc; i++) {
        createSnapshot(argv[i]);
        pid = fork();
        if(pid == -1) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        } else if(pid == 0) {
            // This is the child process
            createSnapshot(argv[i]);
            _exit(EXIT_SUCCESS); // Use _exit in child to prevent flushing of stdio buffers from parent
        }
        printf("Snapshot for %s created succesfully.\n", argv[i]);
        // Parent will continue to the next iteration without waiting here
    }


    // Parent waits for all child processes to finish
    int status;
    int ct = 0;
    while ((pid = wait(&status)) > 0) {
        ct++;
        if(WIFEXITED(status)) {
            printf("Child proccess %d terminated with PID %ld and exit code %d.\n", ct, (long)pid, WEXITSTATUS(status));
        }
    }

    createSnapshot(argv[1]);

    return EXIT_SUCCESS;
}
