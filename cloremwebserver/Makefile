CFLAGS=-Wall -g -pthread #-lsocket
CC=gcc

TARGETS=cloremwebserver

all: $(TARGETS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@
	chmod +x $@

clean:
	rm -f $(TARGETS)
