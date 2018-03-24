// Theodore Bieber
// Distributed Computing Systems
// Project 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <utime.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>

// this is a program to complement the rm and dv programs
// this program is meant to dump the dumpster folder, removing all backups of the files or directories 
// removed using rm.c
// ./dump -h prints the usage information and exits the program
// otherwise, ./dump will just empty the dumpster

// function prototypes
char* concat(char* s1, char* s2);
void removeFolderForce(char* folder);
void usage(void);

// flags for the help option and error detection
int hFlg = 0;
int errFlg = 0;

int main(int argc, char** argv) {
	int c;
	int i;
	extern int optind, opterr;

	// loop through and check for options
	while((c = getopt(argc, argv, "h")) != -1) {
		switch(c) {
			case 'h': 
				if(hFlg) {
					errFlg++;
					break;
				}
				hFlg++;
				break; 
			default: 
				errFlg++;
				break;
		}
	}
	if(errFlg || (argc != optind)) {
		usage();
	}
	if(hFlg) {
		usage();
	}
	char* dumpPath = NULL;
	dumpPath = getenv("DUMPSTER");
	if(!dumpPath) {
		printf("No match for DUMPSTER in environemt\n");
		exit(-1);
	}
	removeFolderForce(dumpPath);
}

// print the usage of the command
void usage(void) {
	fprintf(stderr, "dump - permanently remove files from the dumpster\n");
	fprintf(stderr, "usage: dump [-h]");
	exit(-1);
}

// function to force remove a folder
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
        }
        else if(S_ISDIR(fileStat.st_mode)) {
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

// combine two strings
char* concat(char* s1, char* s2) {
    char* result = malloc(strlen(s1)+strlen(s2)+1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}