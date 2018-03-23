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

char* concat(char* s1, char* s2);
void removeFolderForce(char* folder);
void usage(void);

int hFlg = 0;
int errFlg = 0;

int main(int argc, char** argv) {
	int c;
	int i;
	extern int optind, opterr;
	while((c = getopt(argc, argv, "h")) != -1) {
		switch(c) {
			case 'h': 
				if(hFlg) {
					errFlg ++;
					break;
				}
				hFlg ++;
				break; 
			default: 
				errFlg ++;
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

void usage(void) {
	fprintf(stderr, "dump - permanently remove files from the dumpster\n");
	fprintf(stderr, "usage: dump [-h]");
	exit(-1);
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
            printf("Handling .. and . folder\n");
            d = readdir(dp);
            continue;
        }
        struct stat fileStat;
        // Get the current file.
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

char* concat(char* s1, char* s2) {
    char* result = malloc(strlen(s1)+strlen(s2)+1);
    //in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}