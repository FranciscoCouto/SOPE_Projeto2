CFLAGS= -Wall -g
LFLAGS= -lpthread -lrt
OUTDIR= ./bin/

all: balcao ger_cl

balcao: balcao.c
	gcc $(CFLAGS) -o $(OUTDIR)balcao balcao.c $(LFLAGS)

ger_cl: ger_cl.c
	gcc $(CFLAGS) -o $(OUTDIR)ger_cl ger_cl.c $(LFLAGS)

