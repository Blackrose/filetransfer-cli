
# ARCH has two value: x86 and arm
ARCH=x86
#ARCH=arm

DEBUG=y

CFLAGS= -g -Wall -I./inc

ifeq ($(ARCH), x86)
    CC=gcc
    LDFLAGS= -lsqlite3 -lssl -lcrypto
else
    CC=arm-linux-gcc
    LDFLAGS= -L /armtools/lib/ -static -lssl -lcrypto
endif
ifeq ($(ARCH), x86)
    TARGET=server client
else
    TARGET=server-arm client-arm
endif

ifeq ($(DEBUG), y)
    CFLAGS += -D HAVE_DEBUG
endif

INC_DIR=inc
SRC_DIR=src

SRC_FILES=$(wildcard $(SRC_DIR)/*.c)
OBJSC=$(SRC_DIR)/ftpclient.o $(SRC_DIR)/client_process.o $(SRC_DIR)/common.o
OBJSS=$(SRC_DIR)/ftpserver.o $(SRC_DIR)/serv_client.o $(SRC_DIR)/common.o

all:$(TARGET)
	
server:$(OBJSS)
	$(CC) -o $@ $(LDFLAGS) $?

client:$(OBJSC)
	$(CC) -o $@ $(LDFLAGS) $?

$(SRC_DIR)/%.o:%.c
	$(CC) $(CFLAGS) -o $@ -c $^

.PHONY:clean
clean:
	rm -f $(OBJSC) $(OBJSS)
	rm -f $(TARGET)
