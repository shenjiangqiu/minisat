dir=$(shell pwd)
CPPFLAGS += -g -O0 -Wall
$(dir)/test:$(dir)/assign_wrap_test.o
	g++ $^  -lsjqcache  -o $@
clean:
	rm -rf test *.o