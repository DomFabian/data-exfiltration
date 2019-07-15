CC=gcc

all: encoder decoder

encoder: encoder.c
	$(CC) -o encoder encoder.c $(CFLAGS)

decoder: decoder.c
	$(CC) -o decoder decoder.c $(CFLAGS)

clean:
	rm -f encoder decoder output.png
