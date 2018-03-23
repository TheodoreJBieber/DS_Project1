// Theodore Bieber
// Distributed Computing Systems
// Project 1

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>
#include <utime.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <string.h>
#include <errno.h>


void usage(void);
char* getExtention(char* path);
void getDumpFilePath(char* file, char* dumpPath, char** newPath);
char* concat(char* c1, char* c2);
void copyToDump(char* file, char* dumpPath, struct stat fileStat);
void removeFolder(char* currPath, char* currDumpPath, int isSamePtn);
void removeFolderForce(char* folder);

int fFlg = 0;
int hFlg = 0;
int rFlg = 0;
int errFlg = 0;

int main(int argc, char** argv) {
    int c;
    int i;
    extern int optind, opterr;
    opterr = 1;
    
    while((c = getopt(argc, argv, "fhr")) != -1) {
        switch(c) {
            case 'f':
                if(fFlg)
                {
                    errFlg ++;
                    break;
                }
                fFlg ++;
                break;
            case 'h':
                if(hFlg)
                {
                    errFlg ++;
                    break;
                }
                hFlg ++;
                break;
            case 'r':
                if(rFlg)
                {
                    errFlg ++;
                    break;
                }
                rFlg ++;
                break;
            default:
                errFlg ++;
                break;
        }
    }
    if(errFlg) {
        usage();
    }

    if(hFlg) {
        usage();
    }
    // Get file names.
    int fileCnt = argc - optind;
    if(!fileCnt) {
        printf("rm: missing file name\n");
        usage();
    }
    char* files[fileCnt];
    for(i = 0; i < fileCnt; i++) {
        files[i] = argv[i + optind];
    }

    // Get DUMPSTER environment variable.
    char* dumpPath = NULL;
    dumpPath = getenv("DUMPSTER");
    if(!dumpPath) {
        printf("Missing variable 'DUMPSTER' in environment\n");
        exit(-1);
    }
    // printf("%s\n", dumpPath);
    // Get stat for dumpster.
    struct stat dumpStat;
    int dRtn = stat(dumpPath, &dumpStat);
    // If dRtn is not 0, stat() failed
    if(dRtn) {
        perror("stat() call failed");
        exit(dRtn);
    }
    // Move file to DUMPSTER
    for(i = 0; i < fileCnt; i ++) {
        char* file = files[i];
        // printf("Current processing file is: %s\n", file);
        // Check whether the file exists or not.
        // File not exist, report error and continue to next file.
        if(access(file, F_OK) == -1) {
            printf("File or folder %s does not exist\n", file);
            continue;
        }
        // Get stat for current file.
        struct stat fileStat;
        int fRtn = stat(file, &fileStat);
        if(fRtn) {
            perror("stat() call failed");
            exit(fRtn);
        }
        // TODO: Is -r need to be checked when -f is specified ? 
        if(fFlg) {
            if(S_ISREG(fileStat.st_mode)) {
                int rRtn = remove(file);
                if(rRtn) {
                    perror("remove() call failed");
                    exit(rRtn);
                }
            } else if(S_ISDIR(fileStat.st_mode)) {
                removeFolderForce(file);
                int rRtn = remove(file);
                if(rRtn) {
                    perror("remove() call failed");
                    exit(rRtn);
                }
            }
            continue;
        }

        // Check for partition.
        // Same partition.
        if(fileStat.st_dev == dumpStat.st_dev) {
            // printf("On same partition!\n");
            // Check for file or folder.
            // If it is a file, move it to the dumpster folder.
            if(S_ISREG(fileStat.st_mode)) {
                char* newPath;
                // Get the file path in dumpster.
                getDumpFilePath(file, dumpPath, &newPath);
                int rRtn = rename(file, newPath);
                if(rRtn) {
                    perror("rename() call failed");
                    exit(rRtn);
                }
                int cRtn = chmod(newPath, fileStat.st_mode);
                if(cRtn) {
                    perror("chmod() call failed");
                    exit(cRtn);
                }

            } else if(S_ISDIR(fileStat.st_mode)) {
                // Check for -r flag.
                // If -r is not presented on the folder. 
                if(!rFlg) {
                    printf("-r option missing for folder %s. ", file);
                    printf("Precede to the next file.\n");
                    continue;
                }
                // Remove the other files and folders in this folder.
                removeFolder(file, dumpPath, 1);
                // Remove this folder.
                int rRtn = rmdir(file);
                if(rRtn)
                {
                    perror("rmdir() call failed");
                    exit(rRtn);
                }
            }
        } else {
            // Check for file or folder.
            if(S_ISREG(fileStat.st_mode)) {
                // printf("On different partition!\n");
                // Copy file and set metadata.
                copyToDump(file, dumpPath, fileStat);
                int uRtn = unlink(file);
                if(uRtn) {
                    perror("unlink() call failed");
                    exit(uRtn);
                }
            } else if(S_ISDIR(fileStat.st_mode)) {
                if(!rFlg) {
                    printf("-r option missing for folder %s. ", file);
                    printf("Precede to the next file.\n");
                    continue;
                }
                removeFolder(file, dumpPath, 0);
                int rRtn = rmdir(file);
                if(rRtn) {
                    perror("rmdir() call failed");
                    exit(rRtn);
                }
            }
        }
    }
    return 0;
}

void removeFolderForce(char* folder) {
    DIR* dp;
    struct dirent* d;
    dp = opendir(folder);
    if(dp == NULL) {
        perror("open() call failed");
        exit(-1);
    }
    d = readdir(dp);
    while(d) {
        if((strcmp(d->d_name, "..") == 0) || (strcmp(d->d_name, ".") == 0)) {
            // printf("Handling .. and . folder\n");
            d = readdir(dp);
            continue;
        }
        struct stat fileStat;
        // Get the current file.
        char* currFile = concat(folder, "/");
        currFile = concat(currFile, d->d_name);
        // printf("Current processing file is: %s\n", currFile);
        int sRtn = stat(currFile, &fileStat);
        if(sRtn) {
            perror("stat() call failed");
            exit(sRtn);
        }
        if(S_ISREG(fileStat.st_mode)) {
            int rRtn = remove(currFile);
            if(rRtn) {
                perror("remove() call failed");
                exit(rRtn);
            }
        } else if(S_ISDIR(fileStat.st_mode)) {
            removeFolderForce(currFile);
            int rRtn = rmdir(currFile);
            if(rRtn) {
                perror("unlink() call failed");
                exit(rRtn);
            }
        }
        free(currFile);
        d = readdir(dp);
    }
    closedir(dp);
}

// Write a function to recursively remove a folder.
// current path, dumpster path, flag to check wheter is the same partition.
// currPath and currDumpPath is changed every time removeFolder is called recursively.
void removeFolder(char* currPath, char* currDumpPath, int isSamePtn) {
    DIR* dp;
    struct dirent* d;
    // printf("Directory is %s\n", currPath);
    // printf("Current dumpster path is %s\n", currDumpPath);
    // Update current dumpster path.
    char* newDumpPath;
    getDumpFilePath(currPath, currDumpPath, &newDumpPath);

    // printf("new dumpster path is %s\n", newDumpPath);

    // Get the mode for old folder.
    struct stat srcFolderStat;
    int sRtn = stat(currPath, &srcFolderStat);
    // If dRtn is not 0, stat() failed
    if(sRtn) {
        perror("stat() call failed");
        exit(sRtn);
    }

    // Create a new folder in dumpster and update currDumpPath.
    int mRtn = mkdir(newDumpPath, srcFolderStat.st_mode);
    if(mRtn) {
        perror("mkdir() call failed");
        exit(mRtn);
    }

    dp = opendir(currPath);
    if(dp == NULL) {
        perror("open() call failed");
        exit(-1);
    }
    d = readdir(dp);
    while(d) {
        if((strcmp(d->d_name, "..") == 0) || (strcmp(d->d_name, ".") == 0)) {
            d = readdir(dp);
            continue;
        }
        // Check wheter it is a folder or a file.
        struct stat currStat;
        // Get the current file.
        char* currFile = concat(currPath, "/");
        currFile = concat(currFile, d->d_name);
        int sRtn = stat(currFile, &currStat);
        if(sRtn) {
            perror("stat() call failed");
            exit(sRtn);
        }
        // If it is a file, remove file.
        if(S_ISREG(currStat.st_mode)) {
            if(isSamePtn) {
                char* newPath;
                getDumpFilePath(currFile, newDumpPath, &newPath);
                int rRtn = rename(currFile, newPath);
                if(rRtn) {
                    perror("rename() call failed");
                    exit(rRtn);
                }
                int cRtn = chmod(newPath, currStat.st_mode);
                if(cRtn) {
                    perror("chmod() call failed");
                    exit(cRtn);
                }
            } else {
                copyToDump(currFile, newDumpPath, currStat);
                int uRtn = unlink(currFile);
                if(uRtn)
                {
                    perror("unlink() call failed");
                    exit(uRtn);
                }
            }
        } else if(S_ISDIR(currStat.st_mode)) {
            // If it is a directory.
            removeFolder(currFile, newDumpPath, isSamePtn);
            int rRtn = rmdir(currFile);
            if(rRtn)
            {
                perror("unlink() call failed");
                exit(rRtn);
            }
        }
        free(currFile);
        d = readdir(dp); 
    }
    // unlink dir
    closedir(dp);
    // Set the time and mode for the new folder here.
    int cRtn = chmod(newDumpPath, srcFolderStat.st_mode);
    if(cRtn) {
        perror("chmod() call failed");
        exit(cRtn);
    }
    free(newDumpPath);
}

// Reference: http://stackoverflow.com/questions/8465006/how-to-concatenate-2-strings-in-c
char* concat(char* s1, char* s2) {
    char* result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
    //in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}
// Copying file from another partition to dumpster.
void copyToDump(char* file, char* dumpPath, struct stat fileStat) {
    char* newPath;
    getDumpFilePath(file, dumpPath, &newPath);
    FILE* src;
    FILE* tar;
    size_t bytes;
    char buf[1024];
    src = fopen(file, "r");
    if(src == NULL) {
        printf("Error opening file: %s\n", file);
        exit(-1);
    }
    tar = fopen(newPath, "w");
    if(tar == NULL) {
        printf("Error opening file: %s\n", newPath);
        exit(-1);
    }
    while(bytes = fread(buf, 1, 1024, src)) {
        fwrite(buf, 1, bytes, tar);
    }
    if(ferror(src) || ferror(tar)) {
        printf("Error reading and writing file");
        exit(-1);
    }
    fclose(src);
    fclose(tar);
    int cRtn = chmod(newPath, fileStat.st_mode);
    if(cRtn) {
        perror("chmod() call failed");
        exit(cRtn);
    }
    const struct utimbuf srcTim = {fileStat.st_atime, fileStat.st_mtime};
    int uRtn = utime(newPath, &srcTim);
    if(uRtn) {
        perror("utime() call failed");
        exit(uRtn);
    }
    free(newPath);
    return;
}

// Get the path for the file that will be generated in dumpster.
void getDumpFilePath(char* file, char* dumpPath, char** newPath)
{
    char* basec = strdup(file);
    
    char* bname = basename(basec);
    *newPath = concat(dumpPath, "/");
    *newPath = concat(*newPath, bname);
    char* ext = getExtention(*newPath);

    // if extension != ".0"
    if(strcmp(ext, ".0"))
    {
        *newPath = concat(*newPath, ext);
    }
}

// Get extention if duplicate. return .0 if no duplicate. Report error if full.
char* getExtention(char* path) {
    char* tempPath = path;
    int isDup = 0;
    int num = 1;
    char numArr[2];
    numArr[1] = '\0';
    while(access(tempPath, F_OK) != -1) {
        isDup = 1;
        numArr[0] = num + 48;
        tempPath = concat(path, concat(".", numArr));
        if(num == 10) {
            printf("Dumpster is full!");
            exit(-1);
        }
        num ++;
    }
    if(!isDup) {
        numArr[0] = 48;
        return concat(".", numArr);
    }
    numArr[0] = num - 1 + 48;
    free(tempPath);
    return concat(".", numArr);
}

/*  Print the usage of the command
*/
void usage(void)
{
    fprintf(stderr, "rm - remove file and directory to dumpster\n");
    fprintf(stderr, "usage: rm [-f -h -r] file [file ...]\n");
    fprintf(stderr, "\t-f\tforce a complete remove, files not moved to dumpster\n");
    fprintf(stderr, "\t-h\tdisplay basic usage message\n");
    fprintf(stderr, "\t-r\trecurse directories\n");
    exit(-1);
}