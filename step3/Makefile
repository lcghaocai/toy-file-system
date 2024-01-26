CC=gcc

.PHONY: make_all
make_all:
	make client && make fs && make disk && make clean
fs: fs.o
	$(CC) $^ -o $@

disk: disk.o
	$(CC) $^ -o $@

client: client.o
	$(CC) $^ -o $@

%.o: %.c cse2303header.h
	$(CC) -c $^ 
.PHONY: clean
clean:
	-rm *.o *.gch
