GCC_ARGS = -Wall -std=gnu99 -g
OBJ_FILES = memory.o reader.o printer.o logger.o eval.o buildins.o
run: tests repl
	./tests
	./repl

tests: tests.c $(OBJ_FILES)
	gcc $(GCC_ARGS) tests.c $(OBJ_FILES) -o tests

repl: repl.c $(OBJ_FILES)
	gcc $(GCC_ARGS) repl.c $(OBJ_FILES) -o repl


buildins.o: buildins.h buildins.c memory.o
	gcc $(GCC_ARGS) -c buildins.c

eval.o: eval.h eval.c memory.o logger.o
	gcc $(GCC_ARGS) -c eval.c

printer.o: printer.h printer.c memory.o
	gcc $(GCC_ARGS) -c printer.c

reader.o: reader.h reader.c memory.o
	gcc $(GCC_ARGS) -c reader.c

memory.o: memory.h memory.c logger.o
	gcc $(GCC_ARGS) -c memory.c

logger.o: logger.h logger.c
	gcc $(GCC_ARGS) -c logger.c


clean:
	rm -f *.o repl tests