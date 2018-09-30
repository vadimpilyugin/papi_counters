LIBPATH=libs

.PHONY: all
all: main test

main: main.c perror.c
	gcc -o main main.c perror.c ${LIBPATH}/libpapi.a -I headers

.PHONY: clean
clean:
	rm -f main
	rm -f attach_cpu
	rm -f test_attach
	rm -f testprog

.PHONY: test
test: testprog.c
	gcc -o testprog testprog.c ${LIBPATH}/libpapi.a -I headers
