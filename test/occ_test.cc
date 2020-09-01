
#include <catch2/catch.hpp>
#include "mtl/Vec.h"
#include "core/Solver.h"
#include "core/SolverTypes.h"
#include <fstream>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <vector>
#include <iostream>
using namespace Minisat;

TEST_CASE("occ test", "[basic]")
{
    using deleted=Solver::WatcherDeleted;
    Solver s;
    ClauseAllocator ca;
    OccLists<int,vec<vec<int>>,deleted> m_watcher_list(deleted(ca));
    
}