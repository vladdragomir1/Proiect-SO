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
const char *isolation_dir = "/path/to/isolation_dir";

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

//functie scanare continut malitios
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

    ssize_t bytes_read;
    /*
    char buffer[1024];
    int suspicious = 0;

    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0'; // finalizare buffer
        if (scanForMaliciousContent(buffer)) {
            suspicious = 1;
            break;
        }
    }

    close(fd);
    */
    int pipefd[2], suspicious = 0;

    if (pipe(pipefd) == -1) {
        perror("Pipe failed");
        close(fd);
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("Fork failed");
        close(pipefd[0]);
        close(pipefd[1]);
        close(fd);
        return;
    } else if (pid == 0) {
        // Child process
        close(pipefd[0]);
        // Redirectioneaza stdout
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        // Executa un shell script ce scaneaza file ul pt continut malitios
        execl("script.sh", "script.sh", filePath, NULL);
        perror("execl failed");
        exit(EXIT_FAILURE);
    } else {
        close(pipefd[1]); 
        close(fd);

        char scriptOutput[256];
        while ((bytes_read = read(pipefd[0], scriptOutput, sizeof(scriptOutput) - 1)) > 0) {
            scriptOutput[bytes_read] = '\0';
            if (strstr(scriptOutput, "malicious")) {
                suspicious = 1;
            }
        }
        close(pipefd[0]);
        wait(NULL); 

        if (suspicious) {
            isolateFile(filePath, isolation_dir);
        } else {
            printf("File %s is considered safe.\n", filePath);
        }
    }
}

// functie de izolare file
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
    if (argc < 3) {
        fprintf(stderr, "Usage: %s [output_directory] [isolation_directory] <directory_to_process...>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *output_dir = argv[1];
    isolation_dir = argv[2];

    // Verifica și creeaza directorul de izolare daca nu exista
    struct stat st = {0};
    if (stat(isolation_dir, &st) == -1) {
        if (mkdir(isolation_dir, 0777) == -1) {
            perror("Failed to create isolation directory");
            return EXIT_FAILURE;
        }
    }

    pid_t pid[10];  // vector ce tine id uri child process
    int status, processCount = 0;

    for (int i = 3; i < argc; i++) {
        pid[i - 3] = fork();  // creare proces nou
        if (pid[i - 3] == -1) {
            perror("Fork failed");
            return EXIT_FAILURE;
        } else if (pid[i - 3] == 0) {  // child process
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

//SCRIPT.SH
/*
#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 <file_path>"
    exit 1
fi

KEYWORDS=("corrupt" "dangerous" "risk" "attack" "malware" "malicious")
FILE=$1

for kw in "${KEYWORDS[@]}"; do
    if grep -qi $kw "$FILE"; then
        echo "malicious"
        exit 0
    fi
done

echo "clean"
exit 0
*/
