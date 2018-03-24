// Theodore Bieber
// Distributed Computing Systems
// Project 1

// a program that operates similar to the native "rm" terminal command
// it differs in that it will move the files to a dumpster instead of outright deleting them
// the basic usage is ./rm filename [more_files....], where you can specify filepaths that you want to remove
// you can throw in the -f command to completely subvert the dumpster and force remove things
// and you can use the -r command to recursively remove a directory
// the -h command will print the usage information and then quit out of the program

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>
#include <utime.h>
#include <dirent.h>
#include <libgen.h>

// function prototypes
void usage(void);
char* getExtention(char* path);
void getDumpFilePath(char* file, char* dumpPath, char** newPath);
char* concat(char* c1, char* c2);
void copyToDump(char* file, char* dumpPath, struct stat fileStat);
void removeFolder(char* currPath, char* currDumpPath, int isSamePtn);
void removeFolderForce(char* folder);

// these are flags indicating which options have been selected, as well as an error flag for error detection
int fFlg = 0;
int hFlg = 0;
int rFlg = 0;
int errFlg = 0;

int main(int argc, char** argv) {
    int c;
    int i;
    extern int optind, opterr;
    opterr = 1;
    
    // just loop through and check for options
    while((c = getopt(argc, argv, "fhr")) != -1) {
        switch(c) {
            case 'f':
                if(fFlg) {
                    errFlg++;
                    break;
                }
                fFlg ++;
                break;
            case 'h':
                if(hFlg) {
                    errFlg++;
                    break;
                }
                hFlg++;
                break;
            case 'r':
                if(rFlg) {
                    errFlg++;
                    break;
                }
                rFlg++;
                break;
            default:
                errFlg++;
                break;
        }
    }

    if(hFlg || errFlg) {
        usage();
    }
    // get file names
    int fileCnt = argc - optind;
    if(!fileCnt) {
        printf("rm: missing file name\n");
        usage();
    }
    char* files[fileCnt];
    for(i = 0; i < fileCnt; i++) {
        files[i] = argv[i + optind];
    }

    // get dumpster path from environment variable
    char* dumpPath = NULL;
    dumpPath = getenv("DUMPSTER");
    if(!dumpPath) {
        printf("Missing variable 'DUMPSTER' in environment\n");
        exit(-1);
    }
    // get stat for dumpster
    struct stat dumpStat;
    int dRtn = stat(dumpPath, &dumpStat);
    // if dRtn not 0, stat() failed
    if(dRtn) {
        perror("stat() call failed");
        exit(dRtn);
    }
    // move file to DUMPSTER
    for(i = 0; i < fileCnt; i++) {
        char* file = files[i];
        // check if the file exists or not
        // File doesn't exist, report error and continue to next file
        if(access(file, F_OK) == -1) {
            printf("File or folder %s does not exist\n", file);
            continue;
        }
        // get stat for current file
        struct stat fileStat;
        int fRtn = stat(file, &fileStat);
        if(fRtn) {
            perror("stat() call failed");
            exit(fRtn);
        }
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

        // check for partition
        // same partition
        if(fileStat.st_dev == dumpStat.st_dev) {
            // check if file or folder
            // if it is a file, move it to the dumpster folder
            if(S_ISREG(fileStat.st_mode)) {
                char* newPath;
                // get the file path in dumpster
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
                if(!rFlg) {
                    printf("-r option missing for folder %s. ", file);
                    printf("Precede to the next file.\n");
                    continue;
                }
                // remove the other files and folders in this folder
                removeFolder(file, dumpPath, 1);
                int rRtn = rmdir(file);
                if(rRtn) {
                    perror("rmdir() call failed");
                    exit(rRtn);
                }
            }
        } else {
            // check if file or folder
            if(S_ISREG(fileStat.st_mode)) {
                // copy file and set metadata
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
}

// force remove a folder
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
            d = readdir(dp);
            continue;
        }
        struct stat fileStat;
        // get the current file
        char* currFile = concat(folder, "/");
        currFile = concat(currFile, d->d_name);
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

// recursively remove a folder
void removeFolder(char* currPath, char* currDumpPath, int isSamePtn) {
    DIR* dp;
    struct dirent* d;
    // update current dumpster path
    char* newDumpPath;
    getDumpFilePath(currPath, currDumpPath, &newDumpPath);

    // get the mode for old folder
    struct stat srcFolderStat;
    int sRtn = stat(currPath, &srcFolderStat);
    // if dRtn is not 0, stat() failed
    if(sRtn) {
        perror("stat() call failed");
        exit(sRtn);
    }

    // create a new folder in dumpster and update currDumpPath
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
        // check wheter it is a folder or a file
        struct stat currStat;
        // get the current file
        char* currFile = concat(currPath, "/");
        currFile = concat(currFile, d->d_name);
        int sRtn = stat(currFile, &currStat);
        if(sRtn) {
            perror("stat() call failed");
            exit(sRtn);
        }
        // if it is a file, remove file
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
            // if it is a directory
            removeFolder(currFile, newDumpPath, isSamePtn);
            int rRtn = rmdir(currFile);
            if(rRtn) {
                perror("unlink() call failed");
                exit(rRtn);
            }
        }
        free(currFile);
        d = readdir(dp); 
    }
    // unlink dir
    closedir(dp);
    // set the time and mode for the new folder here
    int cRtn = chmod(newDumpPath, srcFolderStat.st_mode);
    if(cRtn) {
        perror("chmod() call failed");
        exit(cRtn);
    }
    free(newDumpPath);
}

char* concat(char* s1, char* s2) {
    char* result = malloc(strlen(s1)+strlen(s2)+1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

// when the file is on another partition, copy to dumpster
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
    const struct utimbuf srcTim = {
        fileStat.st_atime, fileStat.st_mtime
    };
    int uRtn = utime(newPath, &srcTim);
    if(uRtn) {
        perror("utime() call failed");
        exit(uRtn);
    }
    free(newPath);
    return;
}

// get the path for the file that will be generated in dumpster
void getDumpFilePath(char* file, char* dumpPath, char** newPath) {
    char* basec = strdup(file);
    
    char* bname = basename(basec);
    *newPath = concat(dumpPath, "/");
    *newPath = concat(*newPath, bname);
    char* ext = getExtention(*newPath);

    // if extension != ".0"
    if(strcmp(ext, ".0")) {
        *newPath = concat(*newPath, ext);
    }
}

// get extention if duplicate
// return .0 if no duplicate
// Report an error if full
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
        num++;
    }
    if(!isDup) {
        numArr[0] = 48;
        return concat(".", numArr);
    }
    numArr[0] = num - 1 + 48;
    free(tempPath);
    return concat(".", numArr);
}

// print the basic usage information
void usage(void) {
    fprintf(stderr, "rm - remove file and directory to dumpster\n");
    fprintf(stderr, "usage: rm [-f -h -r] file [file ...]\n");
    fprintf(stderr, "\t-f\tforce a complete remove, files not moved to dumpster\n");
    fprintf(stderr, "\t-h\tdisplay basic usage message\n");
    fprintf(stderr, "\t-r\trecurse directories\n");
    exit(-1);
}