
# ARCH has two value: x86 and arm
ARCH=x86
#ARCH=arm

TARGET=server client
CFLAGS= -g -Wall

ifeq ($(ARCH), "x86")
    CC=gcc
    LDFLAGS= -lsqlite3 -lssl -lcrypto
else
    CC=arm-linux-gcc
    LDFLAGS= -L /armtools/lib/ -static -lssl -lcrypto
endif

all:$(TARGET)
	
server: ftpserver.o  serv_client.o common.o
	$(CC) -o server ftpserver.o serv_client.o common.o $(LDFLAGS) 

client: ftpclient.o client_process.o  common.o
	$(CC) -o client ftpclient.o client_process.o common.o $(LDFLAGS)
.c.o:
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONY:clean
clean:
	rm -f *.o
	rm -f $(PROGRAM)
	rm -f $(ARM)
