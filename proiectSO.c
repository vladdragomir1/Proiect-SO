/*Se primeste un folder ./program folder
Sa se faca un snapshot, se mai ruleaza inca o data sa se prinda ca s-a modificat*/

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
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

    // Open the specified directory
    dir = opendir(directoryPath);
    if(dir == NULL) {
        perror("Unable to open directory");
        exit(EXIT_FAILURE);
    }

    // Open the Snapshot.txt file
    snprintf(filePath, sizeof(filePath), "%s/Snapshot.txt", directoryPath);
    FILE *snapshotFile = open(filePath, O_WRONLY, S_IWUSR);

    if(snapshotFile == NULL) {
        perror("Unable to create snapshot file");
        exit(EXIT_FAILURE);
    }

    // Iterate over each entry in the directory
    while((entry = readdir(dir)) != NULL) {
        snprintf(filePath, sizeof(filePath), "%s/%s", directoryPath, entry->d_name);

        // Get file stats
        if(stat(filePath, &fileInfo) == 0) {
            t = fileInfo.st_mtime;
            tmp = localtime(&t);
            strftime(lastModifiedTime, sizeof(lastModifiedTime), "%Y-%m-%d %H:%M:%S", tmp);
            // Write metadata to Snapshot.txt
            if(S_ISDIR(fileInfo.st_mode)) {
                write(snapshotFile, "Directory: %s, Last Modified: %s\n", entry->d_name, lastModifiedTime);
            } else {
                fprintf(snapshotFile, "File: %s, Size: %ld, Last Modified: %s\n", entry->d_name, fileInfo.st_size, lastModifiedTime);
            }
        }
    }

    //Cleanup
    fclose(snapshotFile);
    closedir(dir);
}

void compare_snapshot {

}

int main(int argc, char *argv[]) {

    if(argc > 10) {
        fprintf(stderr, "Usage: %s \n", argv[0]);
        return EXIT_FAILURE;
    }

    for(int i = 1; i < argc; i++) {
        createSnapshot(argv[i]);
    }

    createSnapshot(argv[1]);

    return EXIT_SUCCESS;
}

