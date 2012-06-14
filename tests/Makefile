GCC_ARGS = -Wall -std=gnu99 -g

tests: eval_test printer_test reader_test logger_test scanner_test output_stream_test bytecode_generator_test custom_atom_test bytecode_compiler_test
	./output_stream_test
	./logger_test
	./scanner_test
	./reader_test
	./printer_test
	./eval_test
	./custom_atom_test
	./bytecode_generator_test
	./bytecode_compiler_test

bytecode_compiler_test: bytecode_compiler_test.c ../bytecode_compiler.h ../bytecode.h ../bytecode_compiler.c test_utils.o test_bytecode_utils.o
	cd ..; make bytecode_compiler.o memory.o reader.o eval.o
	gcc $(GCC_ARGS) bytecode_compiler_test.c test_utils.o test_bytecode_utils.o ../*.o -ldl -o bytecode_compiler_test

bytecode_generator_test: bytecode_generator_test.c ../bytecode_generator.h ../bytecode_generator.c test_utils.o test_bytecode_utils.o
	cd ..; make bytecode_generator.o
	gcc $(GCC_ARGS) bytecode_generator_test.c test_utils.o test_bytecode_utils.o ../*.o -ldl -o bytecode_generator_test

custom_atom_test: custom_atom_test.c ../eval.h ../eval.c test_utils.o
	cd ..; make eval.o printer.o reader.o buildins.o
	gcc $(GCC_ARGS) custom_atom_test.c test_utils.o ../*.o -ldl -o custom_atom_test

eval_test: eval_test.c ../eval.h ../eval.c ../buildins.c test_utils.o
	cd ..; make eval.o printer.o reader.o buildins.o
	gcc $(GCC_ARGS) eval_test.c test_utils.o ../*.o -ldl -o eval_test

printer_test: printer_test.c ../printer.h ../printer.c test_utils.o
	cd ..; make printer.o reader.o
	gcc $(GCC_ARGS) printer_test.c test_utils.o ../printer.o ../logger.o ../reader.o ../output_stream.o ../memory.o ../scanner.o -o printer_test

reader_test: reader_test.c ../scanner.h ../scanner.c ../reader.h ../reader.c ../memory.h ../memory.c test_utils.o
	cd ..; make reader.o
	gcc $(GCC_ARGS) reader_test.c test_utils.o ../reader.o ../scanner.o ../memory.o ../logger.o ../output_stream.o -o reader_test

logger_test: logger_test.c ../output_stream.c ../output_stream.h test_utils.o
	cd ..; make logger.o
	gcc $(GCC_ARGS) logger_test.c test_utils.o ../logger.o ../output_stream.o -o logger_test

scanner_test: scanner_test.c ../scanner.h ../scanner.c test_utils.o
	cd ..; make scanner.o
	gcc $(GCC_ARGS) scanner_test.c test_utils.o ../scanner.o -o scanner_test

output_stream_test: output_stream_test.c ../output_stream.h ../output_stream.c test_utils.o
	cd ..; make output_stream.o
	gcc $(GCC_ARGS) output_stream_test.c test_utils.o ../output_stream.o -o output_stream_test

test_bytecode_utils.o: test_bytecode_utils.h test_bytecode_utils.c test_utils.o
	cd ..; make memory.o logger.o
	gcc $(GCC_ARGS) -c test_bytecode_utils.c

test_utils.o: test_utils.h test_utils.c
	gcc $(GCC_ARGS) -c test_utils.c

clean:
	rm -f *.o *_test core