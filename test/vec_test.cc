
#include <catch2/catch.hpp>
#include "mtl/Vec.h"
#include <fstream>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
TEST_CASE("vec base test", "[basic]")
{
    Minisat::vec<int> m_vec;
    for (int i = 0; i < 1000; i++)
    {
        m_vec.push(i);
    }
    Minisat::vec<int> m_vec_2;
    {
        std::ofstream ofs("vec.bin");
        boost::archive::binary_oarchive oa(ofs);
        oa << m_vec;
    }

    {
        std::ifstream ifs("vec.bin");
        boost::archive::binary_iarchive oi(ifs);
        oi >> m_vec_2;
    }
    REQUIRE(m_vec_2.size() == 1000);
    for (int i = 0; i < 1000; i++)
    {
        REQUIRE(m_vec_2[i] == i);
    }
}
TEST_CASE("vec in vec test", "[advance]")
{
    Minisat::vec<Minisat::vec<int>> m_vec;
    for (int i = 0; i < 1000; i++)
    {

        m_vec.push();
        for (int j = 0; j < 100; j++)
        {
            m_vec[i].push(i + j);
        }
    }

    {
        std::ofstream ofs("vec_in_vec.bin");
        boost::archive::binary_oarchive oa(ofs);
        oa << m_vec;
    }
    Minisat::vec<Minisat::vec<int>> m_vec_2;
    {
        std::ifstream ifs("vec_in_vec.bin");
        boost::archive::binary_iarchive oi(ifs);
        oi >> m_vec_2;
    }
    REQUIRE(m_vec_2.size() == 1000);
    for (int i = 0; i < 1000; i++)
    {
        REQUIRE(m_vec_2[i].size() == 100);
        for (int j = 0; j < 100; j++)
        {
            REQUIRE(m_vec_2[i][j] == i + j);
        }
    }
}