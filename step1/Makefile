CC=gcc

.PHONY: make_all
make_all:
	make disk && make clean
disk: disk.o
	$(CC) $^ -o $@

%.o: %.c cse2303header.h
	$(CC) -c $^ 
.PHONY: clean
clean:
	-rm *.o *.gch
