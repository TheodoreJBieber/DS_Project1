



void usage(void);

int main(int argc, char *argv[]) {

}

/* print a usage message and quit */
void usage(void) {
    fprintf (stderr, "touch - set time of file\n");
    fprintf (stderr, "usage: {-f<name>} [-a<time>] [-m<time>] [-h]\n");
    fprintf (stderr, "\t-f\tfile name to set time\n");
    fprintf (stderr, "\t-a\taccess time to set (seconds since midnight, 01/01/70)\n");
    fprintf (stderr, "\t-m\tmodification time to set (seconds since midnight, 01/01/70)\n");
    fprintf (stderr, "\t-h\tdisplay this help message\n");
    exit(1);
}