CFLAGS+=-Wall -W -ggdb3 -std=gnu++11
CC=g++

.PHONY: all clean clean-rabin

all: final_exe

final_exe: sha_256.c rabin.c lzw_sw.c lzw_hw.cpp chunkdict.c main.cpp
	$(CC) $(CFLAGS) -o $@ $^
   
#lzw_compress.o: 
#	$(CC) $(CFLAGS) -c lzw_compress.cpp -o $@

    
#
#final_exe: sha_256.o rabin-cdc.o main.o 
#	$(CC) $(CFLAGS) -o $@ $^
#        
#sha_256.o: sha_256.c
#	$(CC) $(CFLAGS) -c $^ -o $@
#
#rabin-cdc.o: rabin.c 
#	$(CC) $(CFLAGS) -o $@ $^
#
#main.o: main.c
#	$(CC) $(CFLAGS) -o $@ $^
#
#clean: clean-rabin
#
clean:
	rm -f rabin.o main.o sha_256.o lzw_sw.o final_exe
