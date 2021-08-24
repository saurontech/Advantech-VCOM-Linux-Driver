all: libsslselect.a

ssl_select.o: ssl_select.c ssl_select.h
	gcc -c -o $@ $< -Wall -Werror -lcrypto -lz -ldl -static-libgcc

libsslselect.a: ssl_select.o
	ar rcs $@ $^

clean:
	rm -f ./*.o
	rm -f ./*.a
