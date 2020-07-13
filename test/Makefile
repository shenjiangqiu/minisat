CPPFLAGS+=-std=c++17 -I/home/sjq/git/yy -g
LDFLAGS+=-lboost_serialization -lsjqcache -lz
OBJECTS=main_test.o second_test.o vec_test.o clause_allocator_test.o  occ_test.o solver_test.o
DEPENDENCE=/home/sjq/git/yy/core/Solver.or  /home/sjq/git/yy/core/acc.or  /home/sjq/git/yy/core/acc.or \
/home/sjq/git/yy/core/read_config.or


test:$(OBJECTS)  $(DEPENDENCE)
	g++ $(LDFLAGS) $^ -o $@ 

.PHONY:clean
clean:
	rm -rf $(OBJECTS)
