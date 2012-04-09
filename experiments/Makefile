all: fake_interpreter
	./script

fake_interpreter: fake_interpreter.c
	gcc --std=c99 fake_interpreter.c -o fake_interpreter

clean:
	rm -f fake_interpreter