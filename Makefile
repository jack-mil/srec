CC=gcc
CFLAGS=-Wall -Wextra -Wfloat-equal -Wshadow -Wstrict-overflow=2 -Wwrite-strings -Wswitch-default -Wswitch-enum -O2 -pipe
OBJ = srec.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

srec: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm *.o srec
