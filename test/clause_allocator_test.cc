
#include <catch2/catch_test_macros.hpp>
#include "mtl/Vec.h"
#include "core/SolverTypes.h"
#include <fstream>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <vector>
using namespace Minisat;
TEST_CASE("clause allocator test", "[basic]")
{
    Minisat::ClauseAllocator m_alloc;
    std::vector<int> refs;
    for (int i = 0; i < 1000; i++)
    {
        vec<Lit> lits;
        lits.push(mkLit(1 * i));
        lits.push(mkLit(2 * i));
        auto ref = m_alloc.alloc(lits, false);
        refs.push_back(ref);
    }

    int size = m_alloc.size();

    {
        std::ofstream ofs("clause_alloc.bin");
        boost::archive::binary_oarchive oa(ofs);
        oa << m_alloc;
    }
    Minisat::ClauseAllocator m_alloc_2;

    {
        std::ifstream ifs("clause_alloc.bin");
        boost::archive::binary_iarchive oi(ifs);
        oi >> m_alloc_2;
    }
    REQUIRE(m_alloc_2.size() == size);
    for (auto ref : refs)
    {
        REQUIRE(m_alloc_2[ref].size() == m_alloc[ref].size());//the clause  size match
        auto &ref1 = m_alloc[ref];
        auto &ref2 = m_alloc_2[ref];
        for (int j = 0; j < ref1.size(); j++)
        {
            REQUIRE(ref1[j] == ref2[j]);//each literal in clause eqauls
        }
    }
}