#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "assign_wrap.h"
#include "acc.h"
//#include "event_queue.h"
//#include "event.h"
#include <iostream>
#include <spdlog/spdlog.h>
std::shared_ptr<assign_wrap<int>> shared_null;

TEST_CASE("main test")
{
    spdlog::set_level(spdlog::level::debug);

    assign_wrap_factory af;
    //std::shared_ptr<assign_wrap<int, int, int, int>> shared_null;
    auto w1 = af.create(1, 100, -1, shared_null);
    w1->add_modified_list(10, 1);
    w1->add_modified_list(11, 2);
    w1->add_modified_list(12, 3);
    w1->add_modified_list(13, 4);
    w1->add_modified_list(14, 5);

    //const auto &list = w1->get_modified();

    SECTION("acc")
    {
        auto acc = create_acc<decltype(w1)::element_type>(32, 16, 32, 200, 20, 200, 40, 60);
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
    spdlog::set_level(spdlog::level::debug);


    SECTION("fake test")
    {
        struct Task
        {
            int get_key() const { return id; }
            int get_value() const { return value; }
            int id;

            int value;
        };
        auto comp = [](const Task &t1, const Task &t2) { return t1.id < t2.id; };
        auto m_queue = EventQueue<Task, decltype(comp)>(comp);
        SECTION("first test")
        {
            m_queue.push({1, 2});
            m_queue.push({2, 3});
            REQUIRE(m_queue.get_next_event() == 3);
            REQUIRE(m_queue.get_next_time() == 2);
        }
        SECTION("second test")
        {
            m_queue.push({4, 1});
            m_queue.push({3, 2});
            REQUIRE(m_queue.get_next_event() == 1);
            REQUIRE(m_queue.get_next_time() == 4);
        }
        SECTION("Third test")
        {
            m_queue.push({4, 1});
            m_queue.push({4, 2});
            REQUIRE(m_queue.get_next_event() == 1);
            REQUIRE(m_queue.get_next_time() == 4);
        }
    }
    SECTION("True test for event_queue")
    {
        assign_wrap_factory af;
        auto w1 = af.create(1, 100, -1, shared_null);
        using T = decltype(w1)::element_type;
        w1->add_modified_list(10, 1);
        w1->add_modified_list(11, 2);
        w1->add_modified_list(12, 3);
        w1->add_modified_list(13, 4);
        w1->add_modified_list(14, 5);
        EventQueue<Event<T>, decltype(EventComp<T>)> m_queue(EventComp<T>);
        auto evalue = EventValue<T>(EventType::FinishAndSendClause, 0, 10, w1, HardwareType::ClauseUnit, 1);
        auto event = Event<T>(evalue, 0, 10);
        m_queue.push(event);
        REQUIRE(m_queue.get_next_event().size == 10);
        auto evalue2 = EventValue<T>(EventType::FinishAndSendClause, 11, 10, w1, HardwareType::ClauseUnit, 1);
        auto event2 = Event<T>(evalue2, 0, 9);
        m_queue.push(event2);
        REQUIRE(m_queue.get_next_event().index == 11);
    }
}

TEST_CASE("real acc test")
{
    spdlog::set_level(spdlog::level::debug);

    assign_wrap_factory af;
    SECTION("simple test")
    {

        auto w1 = af.create(1, 100, -1, shared_null, 0);
        w1->add_modified_list(10, 1);
        w1->add_modified_list(11, 2<<9);
        auto w2 = af.create(2, 100, 10, w1, 1);
        w2->add_modified_list(11, 1);
        auto w3 = af.create(3, 100, 11, w2, 3);
        //std::shared_ptr<...> create_acc<...>(int watcher_proc_size, int watcher_proc_num, int clause_proc_num, int miss_latency, int watcher_process_latency, int clause_process_latency)

        auto acc = create_acc<decltype(w1)::element_type>(32, 16, 32, 200, 20, 200, 40, 60);

        acc->push_to_trail(w1);
        acc->push_to_trail(w2);
        acc->push_to_trail(w3);

        acc->set_ready();
        //acc->print_on();
        acc->print_on(1);

        auto cycle = acc->start_sim();
        std::cout << "cycle = " << cycle << std::endl;
        acc->clear();

        REQUIRE(cycle == 1063);
    }
    SECTION("generate conf test")
    {

        auto w1 = af.create(1, 100, -1, shared_null, 0);
        w1->add_modified_list(10, 1<<9);
        w1->add_modified_list(11, 2<<9);
        w1->add_modified_list(12, 1<<9);
        w1->add_modified_list(13, 2<<9);
        auto w2 = af.create(2, 100, 10, w1, 1);
        w2->add_modified_list(11, 1);
        auto w3 = af.create(3, 100, 11, w2, 3);
        //std::shared_ptr<...> create_acc<...>(int watcher_proc_size, int watcher_proc_num, int clause_proc_num, int miss_latency, int watcher_process_latency, int clause_process_latency)

        auto acc = create_acc<decltype(w1)::element_type>(32, 16, 32, 200, 20, 200, 40, 60);

        acc->push_to_trail(w1);
        acc->push_to_trail(w2);
        acc->push_to_trail(w3);
        SECTION("1, first conf")
        {
            w1->set_generated_conf(10);
            acc->set_ready();
            acc->print_on(1);
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
            w3->add_modified_list(11, 10);
            w3->set_generated_conf(11);
            acc->set_ready();
            //acc->print_on();
            auto cycle = acc->start_sim();
            std::cout << "cycle = " << cycle << std::endl;
            REQUIRE(cycle == 1260);
        }
        acc->clear();
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