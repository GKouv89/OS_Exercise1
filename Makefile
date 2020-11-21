make:
	gcc -o enc/enc1 enc/enc1.c
	gcc -o main p/p1.c

clean:
	rm -f enc/enc1 main 
