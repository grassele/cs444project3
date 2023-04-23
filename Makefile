produce_n_consume: produce_n_consume.c
	gcc -Wall -Wextra -o $@ $^ -lpthread
