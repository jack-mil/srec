CC=gcc
# CFLAGS=-I.
OBJ = srec.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

srec: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm *.o srec
