CFLAGS=-std=c11 -pedantic -Wall -Wvla -Werror -D_POSIX_C_SOURCE -D_DEFAULT_SOURCE

PRG = client
UTILS_PATH = ../

all: $(PRG)
        
client: $(UTILS_PATH)utils_v10.h $(UTILS_PATH)utils_v10.o client.c
		cc $(CFLAGS) -o client client.c $(UTILS_PATH)utils_v10.o

utils_v10.o : utils_v10.h utils_v10.c

clean:
		rm -f *.o
		rm -f $(PRG)
