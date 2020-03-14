EXEC      = minisat
DEPDIR    = mtl utils
CPPFLAGS:=-std=c++17 -Wall -g
DIR=/home/sjq/Downloads/minisat/core

$(DIR)/TEST_WRAP:$(DIR)/assign_wrap_test.o
	g++  $^ -o $@ -Wall
clean:
	rm *.o
#include $(MROOT)/mtl/template.mk
