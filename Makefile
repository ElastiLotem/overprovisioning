CFLAGS=-g -Wall -O2

overprovisioning: overprovisioning.c
	gcc -o $@ $< ${CFLAGS}
