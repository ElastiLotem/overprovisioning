CFLAGS=-g -Wall -O2

overprovisioning: overprovisioning.c
	gcc -o $@ $< ${CFLAGS}

noramdbus: noramdbus.c
	gcc -o $@ $< ${CFLAGS} -std=c11
