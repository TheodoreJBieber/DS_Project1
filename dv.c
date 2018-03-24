// Theodore Bieber
// Distributed Computing Systems
// Project 1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <utime.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>

// this program is written in complement with rm.c
// ./dv file [file...] - restores a file or files from the dumpster
// the -h option will print usage information

// function prototypes
void usage(void);
void getSrcFilePath(char* file, char* dumpPath, char** srcPath);
void removeFolder(char* currPath, char* file, int isSamePtn);
char* concat(char* s1, char* s2);
void copyToTar(char* srcPath, char* tarPath, struct stat fileStat);

// flags for the help option and error detection
int hFlg = 0;
int errFlg = 0;

int main(int argc, char** argv)
{
	int c, i;
	extern int optind, opterr;
	opterr = 1;

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
	if(errFlg) {
		usage();
		exit(-1);
	}
	if(hFlg) {
		usage();
	}
	// get file names
	int fileCnt = argc - optind;
	if(!fileCnt) {
		printf("dv: missing file name\n");
		usage();
	}
	char* files[fileCnt];
	for(i = 0; i < fileCnt; i++) {
		files[i] = argv[i + optind];
	}

	// get the dumpster path from environment variable
	char* dumpPath = NULL;
	dumpPath = getenv("DUMPSTER");
	if(!dumpPath) {
		printf("No match for DUMPSTER in environment\n");
		exit(-1);
	}
	// get stat for dumpster
	struct stat dumpStat;
	int sRtn = stat(dumpPath, &dumpStat);
	if(sRtn) {
		perror("stat() call failed");
		exit(sRtn);
	}
	// get information for current directory
	char currDir[1024];
	char* gRtn = getcwd(currDir, 1024);
	if(!gRtn) {
		perror("getcwd() call failed");
		exit(-1);
	}
	// get stat for current working directory
	struct stat currDirStat;
	sRtn = stat(currDir, &currDirStat);
	if(sRtn) {
		perror("stat() call failed");
		exit(sRtn);
	}
	// move file from dumpster to current directory
	// check for partition
	for(i = 0; i < fileCnt; i++) {
		char* file = files[i];
		// check for file existance
		int aRtn = access(file, F_OK);
		if(aRtn == 0) {
			printf("File or folder %s already exists in current directory!\n", file);
			exit(-1);
		}
		// get the file in dumpster
		char* srcPath;
		getSrcFilePath(file, dumpPath, &srcPath);
		// get the file name of the actual file
		char* dupFile = strdup(file);
		char* tarFile;
		char* token;
		while((token = strsep(&dupFile, "/"))) {
			tarFile = strdup(token);
		}
		// use access to check whether field exists
		if(access(srcPath, F_OK) == -1) {
			printf("File or folder %s does not exits\n", srcPath);
			continue;
		}
		struct stat srcFileStat;
		int sRtn = stat(srcPath, &srcFileStat);
		if(sRtn) {
			perror("stat() call failed");
			exit(sRtn);
		}
		// check partition here
		// check file or folder
		// if same partition
		if(dumpStat.st_dev == currDirStat.st_dev) {
			// if it ia s file
			if(S_ISREG(srcFileStat.st_mode)) {
				int rRtn = rename(srcPath, tarFile);
                if(rRtn) {
                    perror("rename() call failed");
                    exit(rRtn);
                }
                int cRtn = chmod(tarFile, srcFileStat.st_mode);
                if(cRtn) {
                    perror("chmod() call failed");
                    exit(cRtn);
                }
			}
			else if(S_ISDIR(srcFileStat.st_mode)) {
				// recursively add new folder back to current directory
				removeFolder(srcPath, tarFile, 1);
				int rRtn = rmdir(srcPath);
                if(rRtn) {
                    perror("rmdir() call failed");
                    exit(rRtn);
                }
			}
		}
		else {
			if(S_ISREG(srcFileStat.st_mode)) {
				copyToTar(srcPath, tarFile, srcFileStat);
				int uRtn = unlink(srcPath);
				if(uRtn) {
                    perror("unlink() call failed");
                    exit(uRtn);
                }
			}
			else if(S_ISDIR(srcFileStat.st_mode)) {
				removeFolder(srcPath, tarFile, 0);
				int rRtn = rmdir(srcPath);
				if(rRtn) {
					perror("rmdir() call failed");
					exit(rRtn);
				}
			}
		}
		
	}

}
// filestat is the file stat from the src file
void copyToTar(char* srcPath, char* tarPath, struct stat fileStat) {
	FILE* src;
	FILE* tar;
	size_t bytes;
	char* buf[1024];
	src = fopen(srcPath, "r");
	if(src == NULL) {
		printf("Error opening file: %s\n", srcPath);
		exit(-1);
	}
	tar = fopen(tarPath, "w");
	if(tar == NULL) {
		printf("Error opening file: %s\n", tarPath);
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
	int cRtn = chmod(tarPath, fileStat.st_mode);
	if(cRtn) {
		perror("chmod() call failed");
		exit(cRtn);
	}
	const struct utimbuf srcTim = {fileStat.st_atime, fileStat.st_mtime};
	int uRtn = utime(tarPath, &srcTim);
	if(uRtn) {
		perror("utime() call failed");
		exit(uRtn);
	}
	return;
}

// currPath should be the file path in dumpster
// the destination is the current working directory
// file is the file being transferred
void removeFolder(char* currPath, char* file, int isSamePtn) {
	DIR* dp;
	struct dirent* d;
	char* tarPath = file;
	struct stat srcFolderStat;
    int sRtn = stat(currPath, &srcFolderStat);
    // if dRtn is not 0, stat() failed
    if(sRtn) {
        perror("stat() call failed");
        exit(sRtn);
    }
    // create a new folder in curretn working directory
    int mRtn = mkdir(tarPath, srcFolderStat.st_mode);
    if(mRtn) {
        perror("mkdir() call failed");
        exit(mRtn);
    }
    // open the directory in dumpster
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
    	struct stat currStat;
    	char* currFile = concat(currPath, "/");
    	currFile = concat(currFile, d->d_name);
    	int sRtn = stat(currFile, &currStat);
    	if(sRtn) {
    		perror("stat() call failed");
    		exit(sRtn);
    	}
    	char* newTarPath = concat(tarPath, "/");
    	newTarPath = concat(newTarPath, d->d_name);
    	if(S_ISREG(currStat.st_mode)) {
    		if(isSamePtn) {
    			int rRtn = rename(currFile, newTarPath);
    			if(rRtn) {
    				perror("rename() call failed");
    				exit(rRtn);
    			}
                int cRtn = chmod(newTarPath, currStat.st_mode);
                if(cRtn) {
                    perror("chmod() call failed");
                    exit(cRtn);
                }
    		}
    		else {
    			// copy file to newTarPath
    			copyToTar(currFile, newTarPath, currStat);
    			int uRtn = unlink(currFile);
				if(uRtn) {
                    perror("unlink() call failed");
                    exit(uRtn);
                }
    		}
    	}
    	else if(S_ISDIR(currStat.st_mode)) {
    		removeFolder(currFile, newTarPath, isSamePtn);
    		int rRtn = rmdir(currFile);
            if(rRtn) {
                perror("rmdir() call failed");
                exit(rRtn);
            }
    	}
    	free(currFile);
    	free(newTarPath);
        d = readdir(dp); 
    }
    closedir(dp);
    int cRtn = chmod(tarPath, srcFolderStat.st_mode);
    if(cRtn) {
        perror("chmod() call failed");
        exit(cRtn);
    }

}

// get the path for the file that is already in dumpster
// srcPath is the path for the file in dumpster
void getSrcFilePath(char* file, char* dumpPath, char** srcPath) {
	*srcPath = concat(dumpPath, "/");
	*srcPath = concat(*srcPath, file);
	return;
}

// print the usage of the command
void usage(void) {
    fprintf(stderr, "dv - retrive file or directory from dumpster\n");
    fprintf(stderr, "usage: dv [-h] file [file ...]\n");
    fprintf(stderr, "\t-h\tdisplay basic usage message\n");
}

// combine two strings
char* concat(char* s1, char* s2) {
    char* result = malloc(strlen(s1)+strlen(s2)+1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}