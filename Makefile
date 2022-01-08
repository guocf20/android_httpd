all: httpd client
LIBS = -lpthread #-lsocket
#CC=gcc
CC=arm-linux-gnueabi-gcc
httpd: httpd.c  log.c
	$(CC) -static  -g -W -Wall -Wno-nonnull -Wno-format  -o $@ $^ $(LIBS)
	tar -cf a.tar htdocs httpd

client: simpleclient.c
	$(CC) -static  -W -Wall -o $@ $<
clean:
	rm httpd a.tar
	
