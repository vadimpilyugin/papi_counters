LIB_VERSION=5.5.0

LIBPATH=libs/papi-${LIB_VERSION}/src

.PHONY: all
all: main test

main: main.c 
	gcc -o main main.c ${LIBPATH}/libpapi.a -I headers -I ${LIBPATH}

.PHONY: clean
clean:
	rm -f main
	rm -f attach_cpu
	rm -f test_attach
	rm -f testprog

.PHONY: test
test: testprog.c
	gcc -o testprog testprog.c ${LIBPATH}/libpapi.a -I headers -I ${LIBPATH}
