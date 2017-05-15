CFLAGS = -Wall -pedantic -g -std=gnu99
MAIN = httpd

all : $(MAIN)

$(MAIN) : httpd.o simple_net.o 
	gcc $(CFLAGS) httpd.o simple_net.o -o httpd

httpd.o : httpd.c httpd.h
	gcc $(CFLAGS) httpd.c -c 

simple_net.o : simple_net.c simple_net.h
	gcc $(CFLAGS) simple_net.c -c

