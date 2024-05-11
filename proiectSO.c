#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h> 

// variabile globale
const char *keywords[] = {"corrupt", "dangerous", "risk", "attack", "malware", "malicious"};
const int num_keywords = 6;
const char *isolation_dir = "/path/to/isolation_dir"; // Set the path for the isolation directory

const char *suspicious_keywords[] = {"corrupted", "dangerous", "risk", "attack", "malware", "malicious"};
const int minimum_line_count = 3;
const int minimum_word_threshold = 1000;
const int minimum_char_threshold = 2000;

// functii
void createSnapshot(const char *directoryPath);
int scanForMaliciousContent(const char *content);
void analyzeFile(char *filePath);
void isolateFile(const char *filePath, const char *isolationPath);

// functie de creare snapshot
void createSnapshot(const char *directoryPath) {
    DIR *dir;
    struct dirent *entry;
    struct stat fileInfo;
    char filePath[1024];
    time_t t;
    struct tm *tmp;
    char lastModifiedTime[50];

    dir = opendir(directoryPath);
    if (dir == NULL) {
        perror("Unable to open directory");
        exit(EXIT_FAILURE);
    }

    char snapshotPath[1024];
    snprintf(snapshotPath, sizeof(snapshotPath), "%s/Snapshot.txt", directoryPath);
    int fd = open(snapshotPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to open snapshot file");
        closedir(dir);
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL) {
        snprintf(filePath, sizeof(filePath), "%s/%s", directoryPath, entry->d_name);
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 && stat(filePath, &fileInfo) == 0) {
            t = fileInfo.st_mtime;
            tmp = localtime(&t); // Convert time_t to struct tm*
            strftime(lastModifiedTime, sizeof(lastModifiedTime), "%Y-%m-%d %H:%M:%S", tmp); // Format time

            char buffer[1024];
            int length = snprintf(buffer, sizeof(buffer), "%s: %s, Size: %ld, Last Modified: %s\n",
                                  S_ISDIR(fileInfo.st_mode) ? "Directory" : "File",
                                  entry->d_name, fileInfo.st_size, lastModifiedTime);
            write(fd, buffer, length);

            if (!S_ISDIR(fileInfo.st_mode)) {
                analyzeFile(filePath);
            }
        }
    }

    close(fd);
    closedir(dir);
}

//functie scanare continut malicios
int scanForMaliciousContent(const char *content) {
    for (int i = 0; i < num_keywords; i++) {
        if (strstr(content, suspicious_keywords[i]) != NULL) {
            return 1; // continut gasit
        }
    }
    return 0; // continut negasit
}

void analyzeFile(char *filePath) {
    int fd = open(filePath, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open file for reading");
        return;
    }

    char buffer[1024];
    ssize_t bytes_read;
    int suspicious = 0;

    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0'; // finalizare buffer
        if (scanForMaliciousContent(buffer)) {
            suspicious = 1;
            break;
        }
    }

    close(fd);

    if (suspicious) {
        isolateFile(filePath, isolation_dir);
    } else {
        printf("File %s is considered safe.\n", filePath);
    }
}

void isolateFile(const char *filePath, const char *isolationPath) {
    char newLocation[1024];
    snprintf(newLocation, sizeof(newLocation), "%s/%s", isolationPath, strrchr(filePath, '/') + 1);
    if (rename(filePath, newLocation) == -1) {
        perror("Failed to move file");
    } else {
        printf("File isolated: %s\n", newLocation);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [directory_path]\n", argv[0]);
        return EXIT_FAILURE;
    }

    pid_t pid[10];  // vector ce tine id uri child process
    int status, processCount = 0;

    for (int i = 1; i < argc; i++) {
        pid[i - 1] = fork();  // creare proces nou
        if (pid[i - 1] == -1) {
            perror("Fork failed");
            return EXIT_FAILURE;
        } else if (pid[i - 1] == 0) {  // child proccess
            printf("Processing directory: %s\n", argv[i]);
            createSnapshot(argv[i]);
            exit(EXIT_SUCCESS);
        } else {
            processCount++;
        }
    }

    // toate child process -> finish
    for (int i = 0; i < processCount; i++) {
        waitpid(pid[i], &status, 0);  // fiecare child process -> exit
        if (WIFEXITED(status)) {
            printf("Child process %d completed with PID %d and exit status %d.\n", i + 1, pid[i], WEXITSTATUS(status));
        }
    }

    return EXIT_SUCCESS;
}
