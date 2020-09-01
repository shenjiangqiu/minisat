dir=$(shell pwd)
CPPFLAGS += -g -O0 -Wall -I..
$(dir)/test:$(dir)/assign_wrap_test.o $(dir)/acc.o
	g++ $^  -lsjqcache  -o $@
clean:
	rm -rf test *.o