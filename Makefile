LIBPATH=libs

.PHONY: all
all: main test

main: main.c perror.c
	gcc -o main main.c perror.c ${LIBPATH}/libpapi.a

.PHONY: clean
clean: 
	rm -f main
	rm -f attach_cpu
	rm -f test_attach

.PHONY: test
test: attach_cpu.c
	gcc -o test_attach attach_cpu.c ${LIBPATH}/libpapi.a
