CFLAGS=-Wall -g -pthread
CC=gcc

TARGETS=dummywget

all: $(TARGETS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@
	chmod +x $@

clean:
	rm -f $(TARGETS)
