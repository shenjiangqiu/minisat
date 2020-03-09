EXEC      = minisat
DEPDIR    = mtl utils
CPPFLAGS:=-std=c++17
TEST_WRAP:assign_wrap_test.o
	g++  $^ -o $@
clean:
	rm *.o
#include $(MROOT)/mtl/template.mk
