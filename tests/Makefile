GCC_ARGS = -Wall -std=gnu99 -g

reader_test: reader_test.c ../scanner.h ../scanner.c ../reader.h ../reader.c ../memory.h ../memory.c test_utils.o
	cd ..; make scanner.o reader.o memory.o logger.o output_stream.o
	gcc $(GCC_ARGS) reader_test.c test_utils.o ../scanner.o ../reader.o ../memory.o ../logger.o ../output_stream.o -o reader_test

logger_test: logger_test.c ../output_stream.c ../output_stream.h test_utils.o
	cd ..; make logger.o
	gcc $(GCC_ARGS) logger_test.c test_utils.o ../logger.o ../output_stream.o -o logger_test

scanner_test: scanner_test.c ../scanner.h ../scanner.c test_utils.o
	cd ..; make scanner.o
	gcc $(GCC_ARGS) scanner_test.c test_utils.o ../scanner.o -o scanner_test

output_stream_test: output_stream_test.c ../output_stream.h ../output_stream.c test_utils.o
	cd ..; make output_stream.o
	gcc $(GCC_ARGS) output_stream_test.c test_utils.o ../output_stream.o -o output_stream_test

test_utils.o: test_utils.h test_utils.c
	gcc $(GCC_ARGS) -c test_utils.c

clean:
	rm -f *.o *_test core