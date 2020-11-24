make:
	gcc -g -c enc/enc1.c
	gcc -g -c p/p1.c
	gcc -g -c shared/shared_memory.c
	gcc -g -c chan/chan.c
	gcc -o chan/chan chan.o shared_memory.o -lpthread -lrt
	gcc -o enc/enc1 enc1.o shared_memory.o -lpthread -lrt -lcrypto
	gcc -o main p1.o shared_memory.o -lpthread -lrt
	rm -f *.o

clean:
	rm -f chan/chan enc/enc1 main 
