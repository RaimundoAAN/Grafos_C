CC     = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2
OBJS   = main.o grafo.o analise_df.o analise_nacional.o

eleicoes: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -lm

%.o: %.c grafo.h analise_df.h analise_nacional.h
	$(CC) $(CFLAGS) -c $<

run: eleicoes
	./eleicoes dados

clean:
	rm -f $(OBJS) eleicoes
