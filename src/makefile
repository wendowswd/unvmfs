CC = gcc
CFLAGS = -W -Wall -O3
OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
TARGET = libunvmfs.a

#CFLAGS += -D_UNVMFS_DEBUG
CFLAGS += -D_UNVMFS_LOG
#CFLAGS += -D_UNVMFS_TIME

all : $(TARGET)

$(TARGET) : $(OBJECTS)
	$(AR) rscv $@ $^

clean:
	$(RM) $(TARGET) $(OBJECTS)

