make:
	gcc -o enc/enc enc/enc_main.c
	gcc -o main main2.c
	gcc -o p/p p/p_main.c
	gcc -o chan/chan chan/chan_main.c
clean: 
	rm -f enc/enc main2 p/p chan/chan
