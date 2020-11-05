make:
	gcc -o execTest/main execTest/main.c
	gcc -o main2 main2.c

clean: 
	rm -f execTest/main main2
