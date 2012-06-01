PROGRAM=server-arm client-arm
CFLAGS= -g -Wall -L /armtools/lib/ -static  -lssl -lcrypto
CC=arm-linux-gcc
CXX=arm-linux-g++


all:$(PROGRAM)
	
server-arm: ftpserver.o  serv_client.o common.o
	$(CC) $(CFLAGS) -o server-arm ftpserver.o serv_client.o common.o

client-arm: ftpclient.o client_process.o  common.o
	$(CC) $(CFLAGS) -o client-arm ftpclient.o client_process.o common.o

.c.o:
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -f *.o
	rm -f $(PROGRAM)

