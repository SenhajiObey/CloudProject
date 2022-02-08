CFLAGS=-std=c11 -pedantic -Wall -Wvla -Werror -D_POSIX_C_SOURCE -D_DEFAULT_SOURCE

PRG = maint server stat
UTILS_PATH = ../

all: $(PRG)
        
maint: maint.c $(UTILS_PATH)utils_v10.h $(UTILS_PATH)utils_v10.o
		cc $(CFLAGS) -o maint maint.c $(UTILS_PATH)utils_v10.o

server: server.c $(UTILS_PATH)utils_v10.h $(UTILS_PATH)utils_v10.o
		cc $(CFLAGS) -o server server.c $(UTILS_PATH)utils_v10.o

stat: stat.c $(UTILS_PATH)utils_v10.h $(UTILS_PATH)utils_v10.o
		cc $(CFLAGS) -o stat stat.c $(UTILS_PATH)utils_v10.o

utils_v10.o: $(UTILS_PATH)utils_v10.c $(UTILS_PATH)utils_v10.h
	$(CC) $(CCFLAGS) -c $(UTILS_PATH)utils_v10.c

clean:
		rm -f *.o
		rm -f $(PRG)
