SEM = -lpthread -lrt
FLAGS = -g -c

all: main main2 enc1 chan enc2
	rm -f shared_memory.o

shared_memory.o: shared/shared_memory.c
	gcc $(FLAGS) shared/shared_memory.c

p1.o: p/p1.c
	gcc $(FLAGS) p/p1.c

enc1.o: enc/enc1.c
	gcc $(FLAGS) enc/enc1.c

enc1: enc1.o shared_memory.o
	gcc -o enc/enc1 enc1.o shared_memory.o $(SEM) -lcrypto
	rm -f enc1.o

chan.o: chan/chan.c
	gcc $(FLAGS) chan/chan.c

chan: chan.o shared_memory.o
	gcc -o chan/chan chan.o shared_memory.o $(SEM)
	rm -f chan.o

enc2.o: enc/enc2.c
	gcc $(FLAGS) enc/enc2.c

enc2: enc2.o shared_memory.o
	gcc -o enc/enc2 enc2.o shared_memory.o $(SEM) -lcrypto
	rm -f enc2.o

p2.o: p/p2.c
	gcc $(FLAGS) p/p2.c

main: p1.o shared_memory.o
	gcc -o main p1.o shared_memory.o $(SEM)
	rm p1.o

main2: p2.o shared_memory.o
	gcc -o main2 p2.o shared_memory.o $(SEM)
	rm p2.o

test:
	gcc -g -c tests/enc1_test.c
	gcc -g -c tests/p1_test.c
	gcc -g -c shared/shared_memory.c
	gcc -o p1 p1_test.o shared_memory.o -lpthread -lrt
	gcc -o enc1 enc1_test.o shared_memory.o -lpthread -lrt
	rm -f *.o

clean:
	rm -f chan/chan enc/enc2 enc/enc1 main main2 *.o
