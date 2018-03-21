
#include <stdio.h>
#include <stdlib.h>		/* for aoit() */
#include <unistd.h>		/* for getopt() */
#include <utime.h>		/* for utime() */
#include <sys/types.h>
#include <stdlib.h>
#include <dirent.h>

void usage(void);


/* Command Line Options:
	-f: force a complete remove, don't move to dumpster
	-h: display a basic help and usage message
	-r: copy directories recursively
	file [file ...] - one or more files to be removed
*/

char *dumpster_path = "~/dumpster/";

int main(int argc, char *argv[]) {
	char c;
	int recurse = 0;

	while ((c = getopt (argc, argv, "fhr")) != EOF) {
    	switch (c) {
    		case 'f':
      			file = optarg;
      			break;
    		case '?': /* returns ? if there is a getopt error */
    		case 'h':
      			usage();
      			break;
      		case 'r':
      			recurse = 1;
      			break;
    	}
  	}
  	/*
		now check optind for file paths to be removed
  	*/
  	int count;
  	for(count = optind; count < argc; count++) {
  		// remove or rmdir
  	}
}

int remove_file(char *basename, char *dirname) {

}

int recursive_remove(char *basename, char *dirname) {
	DIR *dp;
  	struct dirent *d;

  	dp = opendir(".");
  	if (dp == NULL) {
    	perror("open");
    	exit(1);
  	}

  	d = readdir(dp);
  	while (d) {
    	


    	d = readdir(dp);
  	}

  	closedir(dp);
}




/* print a usage message and quit */
void usage(void) {
    fprintf (stderr, "rm - remove \n");
    fprintf (stderr, "usage: {-f<name>} [-a<time>] [-m<time>] [-h]\n");
    fprintf (stderr, "\t-f\tfile name to set time\n");
    fprintf (stderr, "\t-a\taccess time to set (seconds since midnight, 01/01/70)\n");
    fprintf (stderr, "\t-m\tmodification time to set (seconds since midnight, 01/01/70)\n");
    fprintf (stderr, "\t-h\tdisplay this help message\n");
    exit(1);
}