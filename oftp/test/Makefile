PROGRAM=server client
CFLAGS= -g
CC=gcc

all:$(PROGRAM)
	
server: ftpserver.o  serv_client.o common.o
	$(CC) ${CFLAGS} -o server ftpserver.o serv_client.o common.o

client: ftpclient.o client_process.o  common.o
	$(CC) -o client ftpclient.o client_process.o common.o

.c.o:
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -f *.o
	rm -f $(PROGRAM)

