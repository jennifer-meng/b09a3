CC=gcc
CFLAGS=-c -Wall
LDFLAGS=
SOURCES=stats_functions.c main.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=stats

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

%.o: %.c stats_functions.h
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(OBJECTS) $(EXECUTABLE)

.PHONY: all clean