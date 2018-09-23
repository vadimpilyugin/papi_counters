LIBPATH=libs

.PHONY: all
all: main

main: main.c perror.c
	gcc -o main main.c perror.c ${LIBPATH}/libpapi.a

.PHONY: clean
clean: 
	rm -f main