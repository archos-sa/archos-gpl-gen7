CC=gcc

sambafsmon.so.1.0.1: libc_wrapper.o
	$(CC) -shared -Wl,-soname,sambafsmon.so.1 -o sambafsmon.so.1.0.1 libc_wrapper.o -lc -ldl

libc_wrapper.o: libc_wrapper.c
	$(CC) -fPIC -rdynamic -g -c -Wall libc_wrapper.c

clean:
	-rm libc_wrapper.o sambafsmon.so.1.0.1 *~