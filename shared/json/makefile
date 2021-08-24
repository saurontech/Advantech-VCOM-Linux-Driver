all: libjason.a

jstree.o: jstree.c jsmn.c
	gcc -c -o $@ $< -Wall -Werror
jsmn.o: jsmn.c jstree.c
	gcc -c -o $@ $< -Wall -Werror

libjason.a: jsmn.o jstree.o
	ar rcs $@ $^

clean:
	rm -f ./*.o
	rm -f ./*.a
