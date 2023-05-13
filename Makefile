CFLAGS = -Wall -Werror -g
CC = gcc $(CFLAGS)

swish: swish.c string_vector.o swish_funcs.o swish_funcs_provided.o
	$(CC) -o $@ $^

string_vector.o: string_vector.h string_vector.c
	$(CC) -c string_vector.c

swish_funcs.o: string_vector.o swish_funcs.c
	$(CC) -c swish_funcs.c

clean:
	rm -f swish string_vector.o swish_funcs.o

test-setup:
	@chmod u+x testius
	@rm -f out.txt

ifdef testnum
test: test-setup swish
	./testius test_cases/tests.json -v -n $(testnum)
else
test: swish
	./testius test_cases/tests.json
endif

clean-tests:
	rm -rf test_results out.txt

zip:
	@echo "ERROR: You cannot run 'make zip' from the part2 subdirectory. Change to the main proj3-code directory and run 'make zip' there."
