TARGETS+=libcparam.so
libcparam.so: cparam.h cparam.c
	cc -shared -o libcparam.so cparam.c

TARGETS+=cparam.o
cparam.o: cparam.c cparam.h

TARGETS+=libcparam.a
libcparam.a: cparam.o
	ar -rcs libcparam.a cparam.o

TARGETS+=cparam_demo
cparam_demo: cparam_demo.c libcparam.a
	clang -o cparam_demo cparam_demo.c -L. -lcparam

.PHONY: lib
lib: libcparam.so

.PHONY: demo
demo: cparam_demo

.PHONY: clean
clean:
	rm -f ${TARGETS}

.PHONY: push
push:
	git push -u origin master
