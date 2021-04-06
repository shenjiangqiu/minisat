#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include "assign_wrap.h"
#include "acc.h"
//#include "event_queue.h"
//#include "event.h"
#include <iostream>


using namespace MACC;
TEST_CASE("main test")
{

    assign_wrap_factory af;
    //std::shared_ptr<assign_wrap<int, int, int, int>> nullptr;
    auto w1 = af.create(1, 100, -1, nullptr);
    w1->add_clause_addr(10, 1);
    w1->add_clause_addr(11, 2);
    w1->add_clause_addr(12, 3);
    w1->add_clause_addr(13, 4);
    w1->add_clause_addr(14, 5);

    //const auto &list = w1->get_modified();

    SECTION("acc")
    {
        auto acc = create_acc(32, 16, 32, 200, 20, 200, 40, 60, false, -1);
        acc->push_to_trail(w1);
        auto w2 = af.create(2, 100, 0, w1, w1->get_level() + 1);
        REQUIRE(w2->get_level() == 1);
        auto w3 = af.create(2, 100, 0, w2, w2->get_level() + 1);
        REQUIRE(w3->get_level() == 2);
        acc->push_to_trail(w2);
        acc->push_to_trail(w3);
        REQUIRE(acc->get_max_level() == 2);
    }
}

TEST_CASE("EventQueue")
{

    SECTION("True test for event_queue")
    {
        assign_wrap_factory af;
        auto w1 = af.create(1, 100, -1, nullptr);

        w1->add_clause_addr(10, 1);
        w1->add_clause_addr(11, 2);
        w1->add_clause_addr(12, 3);
        w1->add_clause_addr(13, 4);
        w1->add_clause_addr(14, 5);
        EventQueue m_queue;
        auto evalue = EventValue(EventType::FinishAndSendClause, 0, 10, w1, HardwareType::ClauseUnit, 1);
        auto event = Event(evalue, 0, 10);
        m_queue.push(event);
        REQUIRE(m_queue.get_next_event().size == 10);
        auto evalue2 = EventValue(EventType::FinishAndSendClause, 11, 10, w1, HardwareType::ClauseUnit, 1);
        auto event2 = Event(evalue2, 0, 9);
        m_queue.push(event2);
        REQUIRE(m_queue.get_next_event().index == 11);
    }
}

TEST_CASE("real acc test")
{

    assign_wrap_factory af;
    SECTION("simple test")
    {

        auto w1 = af.create(1, 100, -1, nullptr, 0);
        w1->add_clause_addr(10, 1 << 9);
        w1->add_detail(10, 12345);
        w1->add_detail(10, 12346);
        w1->add_detail(10, 22347);

        w1->add_clause_addr(11, 2 << 9);
        w1->add_detail(11, 112345);
        w1->add_detail(11, 112346);
        w1->add_detail(11, 122347);
        w1->add_detail(11, 312345);
        w1->add_detail(11, 312346);
        w1->add_detail(11, 322347);
        auto w2 = af.create(2, 100, 10, w1, 1);
        w2->add_clause_addr(12, 3 << 9);
        w2->add_detail(12, 412345);
        w2->add_detail(12, 412346);
        w2->add_detail(12, 422347);
        auto w3 = af.create(3, 100, 12, w2, 3);
        //std::shared_ptr<...> create_acc<...>(int watcher_proc_size, int watcher_proc_num, int clause_proc_num, int miss_latency, int watcher_process_latency, int clause_process_latency)

        auto acc = create_acc(32, 16, 32, 200, 20, 200, 40, 60, false, -1);

        acc->push_to_trail(w1);
        acc->push_to_trail(w2);
        acc->push_to_trail(w3);

        acc->set_ready();
        //acc->print_on();
        //acc->print_on(1);

        auto cycle = acc->start_sim();
        std::cout << "cycle = " << cycle << std::endl;
        acc->clear();

        REQUIRE(cycle == 1063);
    }
    SECTION("generate conf test")
    {

        auto w1 = af.create(1, 100, -1, nullptr, 0);
        w1->add_clause_addr(10, 1 << 9);
        w1->add_clause_addr(11, 2 << 9);
        w1->add_clause_addr(12, 1 << 9);
        w1->add_clause_addr(13, 2 << 9);
        auto w2 = af.create(2, 100, 10, w1, 1);
        w2->add_clause_addr(11, 1);
        auto w3 = af.create(3, 100, 11, w2, 3);
        //std::shared_ptr<...> create_acc<...>(int watcher_proc_size, int watcher_proc_num, int clause_proc_num, int miss_latency, int watcher_process_latency, int clause_process_latency)

        auto acc = create_acc(32, 16, 32, 200, 20, 200, 40, 60, false, -1);

        acc->push_to_trail(w1);
        acc->push_to_trail(w2);
        acc->push_to_trail(w3);
        SECTION("1, first conf")
        {
            w1->set_generated_conf(10);
            acc->set_ready();
            //acc->print_on(1);
            auto cycle = acc->start_sim();
            std::cout << "cycle = " << cycle << std::endl;
            REQUIRE(cycle == 420);
        }
        SECTION("2, second conf")
        {
            w2->set_generated_conf(11);
            acc->set_ready();
            //acc->print_on();
            auto cycle = acc->start_sim();
            std::cout << "cycle = " << cycle << std::endl;
            REQUIRE(cycle == 840);
        }
        SECTION("3, third conf")
        {
            w3->add_clause_addr(11, 10);
            w3->set_generated_conf(11);
            acc->set_ready();
            //acc->print_on();
            auto cycle = acc->start_sim();
            std::cout << "cycle = " << cycle << std::endl;
            REQUIRE(cycle == 1260);
        }
        acc->clear();
    }
    SECTION("4, mode2 test")
    {
        auto acc = create_acc(32, 1, 16, 119, 32, 200, 100, 1, true, 19);
        auto w1 = af.create(1, 100, -1, nullptr, 0);
        w1->add_clause_addr(10, 1 << 7);
        w1->add_clause_addr(11, 2 << 7);
        w1->add_clause_addr(12, 3 << 7);
        w1->add_clause_addr(13, 4 << 7);
        auto w2 = af.create(2, 100, 10, w1, w1->get_level()+1);
        w2->add_clause_addr(11, 1);
        acc->push_to_trail(w1);
        acc->push_to_trail(w2);
        acc->start_sim();
    }
    SECTION("parralle test")
    {
        //todo generate multiple assigments at the same time, those assignment should be able to run in parallel
    }
    SECTION("limited hardware test")
    {
        //todo now we have more task and limited hardware resource, the test should be able to queued.
    }
}