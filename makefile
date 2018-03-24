# Theodore Bieber
# Distributed Computing Systems
# Project 1

all: rm dv dump

rm: rm.c
	gcc -o rm rm.c
	
dv: dv.c
	gcc -o dv dv.c
	
dump: dump.c
	gcc -o dump dump.c
	
clean:
	rm rm dv dump