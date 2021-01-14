CC = gcc
OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
TARGET = pattern
CFLAGS = -g $(INCLUDE)

$(TARGET):$(OBJECTS)
	gcc -o $@ $^ $(CFLAGS) $(LIBS) $(LIBPATH)

clean:
	rm -f $(TARGET) $(OBJECTS)


