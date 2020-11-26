make:
	gcc -g -c enc/enc1.c
	gcc -g -c p/p1.c
	gcc -g -c shared/shared_memory.c
	gcc -g -c chan/chan.c
	gcc -o chan/chan chan.o shared_memory.o -lpthread -lrt
	gcc -o enc/enc1 enc1.o shared_memory.o -lpthread -lrt -lcrypto
	gcc -o main p1.o shared_memory.o -lpthread -lrt
	gcc -g -c enc/enc2.c
	gcc -g -c p/p2.c
	gcc -o enc/enc2 enc2.o shared_memory.o -lpthread -lrt -lcrypto
	gcc -o main2 p2.o shared_memory.o -lpthread -lrt
	rm -f *.o

test:
	gcc -g -c tests/enc1_test.c
	gcc -g -c tests/p1_test.c
	gcc -g -c shared/shared_memory.c
	gcc -o p1 p1_test.o shared_memory.o -lpthread -lrt
	gcc -o enc1 enc1_test.o shared_memory.o -lpthread -lrt
	rm -f *.o


clean:
	rm -f chan/chan enc/enc1 main main2
