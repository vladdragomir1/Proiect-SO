#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>

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
    int snapshotFile = open(filePath, O_WRONLY, S_IWUSR); 

    if(snapshotFile < 0) {
        perror("Unable to create snapshot file");
        exit(EXIT_FAILURE);
    }

    // Iterate over each entry in the directory
    while((entry = readdir(dir)) != NULL) {
        snprintf(filePath, sizeof(filePath), "%s/%s", directoryPath, entry->d_name);

        if(stat(filePath, &fileInfo) == 0) {
            t = fileInfo.st_mtime;
            tmp = localtime(&t);
            strftime(lastModifiedTime, sizeof(lastModifiedTime), "%Y-%m-%d %H:%M:%S", tmp);
            // Write metadata to Snapshot.txt
            if(S_ISDIR(fileInfo.st_mode)) {
                fprintf(snapshotFile, "Directory: %s, Last Modified: %s\n", entry->d_name, lastModifiedTime);
            } else {
                fprintf(snapshotFile, "File: %s, Size: %ld, Last Modified: %s\n", entry->d_name, fileInfo.st_size, lastModifiedTime);
            }
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {

    if(argc > 10) {
        fprintf(stderr, "Usage: %s [directory_path]...\n", argv[0]);
        return EXIT_FAILURE;
    }

    int pid;
    for(int i = 1; i < argc; i++) {
        pid = fork();
        
        if(pid == -1) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        } else if(pid == 0) {
            // This is the child process
            createSnapshot(argv[i]);
            printf("Snapshot for %s created succesfully.\n", argv[i]);
            _exit(EXIT_SUCCESS); // Use _exit in child to prevent flushing of stdio buffers from parent
        }
    }
    
    // Parent waits for all child processes to finish
    int status;
    while ((pid = wait(&status)) > 0) {
        if(WIFEXITED(status)) {
            printf("Child terminated with PID %ld and exit code %d.\n", (long)pid, WEXITSTATUS(status));
        }
    }

    return EXIT_SUCCESS;
}
