#ifndef ACC_H
#define ACC_H
#include <math.h>
#include <algorithm>
#include <signal.h>
#include <unistd.h>
#include <memory>
#include <iostream>
#include <queue>
#include <cache.h>
#include <spdlog/spdlog.h>
#include <list>
#include "core/assign_wrap.h"
#include <set>
#include <unordered_set>
namespace MACC
{
    //to do list:
    /**
 * 1,implement vault, different clause should be assigned to different vault.
 * 2, collect vault_im_balance
 * 3, implement push to other watcher list
 * 4, implement new_assignment confilict.
 * 
 * 
 * 
 * 
**/
    namespace sjq
    {
        template <typename _Tp, typename _Sequence = std::deque<_Tp>>

        class queue : public std::queue<_Tp, _Sequence>
        {
        public:
            const auto &get_container() const
            {
                return std::queue<_Tp, _Sequence>::c;
            }
            auto cbegin() const
            {
                return std::queue<_Tp, _Sequence>::c.cbegin();
            }
            auto cend() const
            {
                return std::queue<_Tp, _Sequence>::c.cend();
            }
            auto begin()
            {
                return std::queue<_Tp, _Sequence>::c.begin();
            }
            auto end()
            {
                return std::queue<_Tp, _Sequence>::c.end();
            }
        };
    } // namespace sjq
    enum class EventType
    {
        FinishedReadClause, //first read clause, this shoud run in parallel, and then send processclause , this should be in sequencial
        ReadWatcherList,
        ProcessWatcherList,
        FinishAndSendClause,
        SendToMemCtr, //only valid on mode 2
        ProcessClause,
        missAccess,
        sendToVault,
        VaultMissAccess,
        FinishedInMemCtr,
        FinishedModWatcherList //new type here

    };
    enum class HardwareType
    {
        watcherListUnit,
        ClauseUnit
    };
    struct EventValue
    {
        EventValue(EventType _tp, int _id, int _sz, assign_wrap *v,
                   HardwareType ht, int hi,
                   unsigned long long addr,
                   int vault_index,
                   int watcher_index) : type(_tp),
                                        index(_id),
                                        size(_sz), value(v),
                                        hType(ht),
                                        HardwareId(hi),
                                        addr(addr),
                                        vault_index(vault_index),
                                        watcher_index(watcher_index) {}
        EventType type;
        int index;
        int size;
        assign_wrap *value; // the assignment wraper
        HardwareType hType;
        int HardwareId;
        unsigned long long addr;
        int vault_index;
        int watcher_index;
        bool is_miss;
    };

    std::ostream &operator<<(std::ostream &os, EventType type);

    class Event
    {
    public:
        Event(const EventValue &value, int startTime, int endTime) : start_time(startTime), end_time(endTime), mEventValue(value) {}
        int get_key() const { return end_time; }
        const EventValue &get_value() const { return mEventValue; }
        EventValue &get_value_ref() { return mEventValue; }

        int get_start_time() const { return start_time; }
        int get_end_time() const { return end_time; }
        bool operator<(const Event &other) const { return end_time > other.end_time; }
        void set_start_time(int t_start_time) { start_time = t_start_time; }
        void set_end_time(int t_end_time) { end_time = t_end_time; }
        void set_value(const EventValue &value) { mEventValue = value; }

    private:
        int start_time;
        int end_time;
        EventValue mEventValue;
    };

    class EventQueue
    {
    public:
        EventQueue() : m_event_queue() {}
        void push(const Event &event) { m_event_queue.push(event); }
        const Event &top() const { return m_event_queue.top(); }
        void pop() { m_event_queue.pop(); }
        bool empty() const { return m_event_queue.empty(); }
        auto get_next_time() const
        {
            return m_event_queue.top().get_key();
        }
        auto get_next_event() const
        {
            return m_event_queue.top().get_value();
        }
        auto get_next() const { return m_event_queue.top(); }
        void clear()
        {
            while (!m_event_queue.empty())
                m_event_queue.pop();
        }
        size_t size() const { return m_event_queue.size(); }

    private:
        std::priority_queue<Event, std::deque<Event>> m_event_queue;
    };

    class derectory
    {
    public:
        enum stats
        {
            hit_local,
            hit_remote,
            miss
        };
        void write(unsigned long long addr, bool miss)
        {
        }
    };

    std::ostream &operator<<(std::ostream &os, const EventValue &eventValue);

    std::ostream &operator<<(std::ostream &os, const Event &event);
    struct memory_bendwidth_info
    {
        int num_cycles_for_watcher;
        int num_cycles_for_clause;
    };

    class BandWidthManager
    {
    public:
        BandWidthManager(unsigned total_bandwidth) : current_bandwidth(0), total_bandwidth(total_bandwidth)
        {
        }
        bool empty()
        {
            return current_bandwidth == 0;
        }
        void print()
        {
            std::cout << "current usage:" << current_bandwidth << std::endl;
            std::cout << "total usage:" << total_bandwidth << std::endl;
        }
        bool tryAddUse(unsigned added_bandwidth)
        {
            if (current_bandwidth + added_bandwidth > total_bandwidth)
            {
                return false;
            }
            current_bandwidth += added_bandwidth;
            return true;
        }
        void delUse(unsigned deled_bandwidth)
        {
            current_bandwidth -= deled_bandwidth;
        }

    private:
        unsigned current_bandwidth;
        const unsigned total_bandwidth;
        friend std::ostream &operator<<(std::ostream &os, BandWidthManager &);
    };
    std::ostream &operator<<(std::ostream &os, BandWidthManager &bm);

    class ACC
    {
    public:
        ACC() = delete;
        ACC(int watcher_proc_size,
            int watcher_proc_num,
            int clause_proc_num,
            int miss_latency,
            int watcher_process_latency,
            int clause_process_latency,
            int vault_memory_access_latency,
            int cpu_to_vault_latency,
            bool mode2 = false,
            int ctr_latency = -1);
        ~ACC();
        friend std::ostream &operator<<(std::ostream &, const ACC &);
        void print_on(int level)
        {
            print_level = level;
        }
        void print_off()
        {
            print_level = 0;
        }
        bool ready() const
        {
            return m_ready;
        }
        void set_ready()
        {
            m_ready = true;
        }
        void clear()
        {
            current_cache_time = 0;
            m_ready = false;
            value_queue.clear();
            value_set.clear();
            //time_records.clear();
            assert(m_event_queue.empty());
            //m_event_queue.clear();
            //watcher_access.clear();
            //clause_access.clear();
            assert(m_cache.get_on_going_misses() == 0);
            assert(std::all_of(vault_waiting_queue.begin(), vault_waiting_queue.end(), [](auto &the_queue) { return the_queue.empty(); }));

            assert(std::all_of(vault_busy.begin(), vault_busy.end(), [](bool a) { return a == false; }) == true);
        }

        int start_sim();
        void handle_vault_process(int vault_index, int end_time);

        //this function simulate:
        //   1.when the in_order core start to process the clause
        //   2.it generate the miss event to the memory controller.
        //   3.when controller recived the request, the process them oneby one,
        //   4.the controller process event need to calculate the end time of the process of the clause.
        //   5.No Other Assumption
        void handle_vault_process_mode2(int vault_index, int end_time);

        void push_to_trail(assign_wrap *value)
        {
            value_queue.push_back(value);
            value_set.insert(value);
        }
        int get_trail_size() const
        {
            return value_queue.size();
        }
        void handle_new_watch_list(std::queue<std::pair<int, assign_wrap *>> &waiting_queue, unsigned long long end_time, int watcher_index);

        bool have_watcher_unit_ready() const { return m_using_watcher_unit < w_num; }
        bool have_clause_unit_ready() const { return m_using_watcher_unit < c_num; }
        void value_push(assign_wrap *value) { value_queue.push_back(value); }
        int get_max_level() { return (*std::prev(value_queue.cend()))->get_level(); }
        //void push_time_records(assign_wrap*<Time_record> time) { time_records.push_back(time); }
        unsigned long long get_global_blocked_clause() const { return global_blocked_clause; }
        unsigned long long get_global_blocked_times() const { return global_blocked_times; }
        unsigned long long get_waiting_watcher_list() const { return waiting_watcher_list; }
        unsigned long long get_waiting_watcher_times() const { return waiting_watcher_times; }
        unsigned long long get_idle_clause_unit_total() const { return idle_clause_unit_total; }
        unsigned long long get_idle_clause_unit_times() const { return idle_clause_unit_times; }
        unsigned long long get_idel_watcher_total() const { return idel_watcher_total; }
        unsigned long long get_idel_watcher_times() const { return idel_watcher_times; }
        unsigned long long get_m_access() const { return m_access; }
        unsigned long long get_m_miss() const { return m_miss; }
        unsigned long long get_m_hit() const { return m_hit; }
        unsigned long long get_m_hit_res() const { return m_hit_res; }
        void print() const;
        int assign_to_vault(unsigned long long addr) { return (addr >> 7) & (c_num - 1); }
        //auto &get_watcher_map() { return watcher_access; }
        //auto &get_clause_map() { return clause_access; }
        unsigned long long get_total_read_old_times() { return total_read_old_times; }
        unsigned long long get_total_write_same_times() { return total_write_same_times; }
        unsigned long long get_total_write_conflict_times() { return total_write_conflict_times; }

        unsigned long long get_total_reads() { return total_reads; }
        unsigned long long get_total_writes() { return total_writes; }

    private:
        unsigned long long current_cache_time = 0;
        //std::unordered_map<unsigned long long, int> watcher_access;
        //std::unordered_map<unsigned long long, int> clause_access;
        void add_clause_read_set(assign_wrap *value, int index);
        void add_clause_write_set(assign_wrap *value, int index);
        int get_total_write_times(int literal);
        int get_total_read_times(int literal);
        void update_read_write_conflict();
        void clear_read_write_set();
        BandWidthManager l3_bandwidth_manager;
        BandWidthManager dram_bandwith_manager;
        std::vector<int> clause_buffer_size;
        //int vault_memory_access_latency;
        void mem_ctr_process(Event event, int end_time);
        int w_size;
        int w_num;
        int c_num;
        bool m_ready;
        std::vector<assign_wrap *> value_queue;
        std::set<assign_wrap *> value_set;
        //std::vector<assign_wrap*<Time_record>> time_records;
        int miss_latency;
        int watcher_process_latency;
        int clause_process_latency;
        int m_using_watcher_unit;
        int m_using_clause_unit;

        EventQueue m_event_queue;
        bool m_memory_ctr_busy = false;
        int print_level = 0;
        struct vault_waiting_queue_value
        {
            vault_waiting_queue_value(int index, assign_wrap *value, unsigned long long addr) : index(index), value(value), addr(addr) {}
            int index;
            assign_wrap *value;
            unsigned long long addr;
        };
        std::vector<std::queue<vault_waiting_queue_value>> vault_waiting_queue;
        std::vector<cache> vault_cache;
        //statistics
        cache m_cache;
        std::vector<bool> vault_busy;
        int vault_memory_access_latency;
        int cpu_to_vault_latency;
        bool mode2;
        int ctr_latency;
        std::queue<Event> mem_ctr_queue;
        unsigned long long global_blocked_clause = 0;
        unsigned long long global_blocked_times = 0;
        unsigned long long waiting_watcher_list = 0;
        unsigned long long waiting_watcher_times = 0;
        unsigned long long idle_clause_unit_total = 0;
        unsigned long long idle_clause_unit_times = 0;
        unsigned long long idel_watcher_total = 0;
        unsigned long long idel_watcher_times = 0;
        unsigned long long m_access = 0;
        unsigned long long m_miss = 0;
        unsigned long long m_hit = 0;
        unsigned long long m_hit_res = 0;
        std::vector<unsigned long long> vault_waiting_all;
        std::vector<unsigned long long> vault_waiting_times;
        std::vector<unsigned long long> vault_idle_all;
        std::vector<unsigned long long> vault_idle_times;
        std::vector<bool> watcher_busy;
        std::vector<bool> c_busy;
        std::vector<int> next_c;
        std::vector<std::queue<std::pair<int, assign_wrap *>>> clause_read_waiting_queue;
        unsigned long long vault_tasks = 0;
        unsigned int current_running_level = 0;

        //std::unordered_set<unsigned long long> clause_foot_print_set;
        //unsigned long long total_clause_foot_print = 0;
        //std::unordered_set<unsigned long long> watcher_list_foot_print_set;
        //unsigned long long total_watcher_list_foot_print = 0;

        unsigned long long watcher_min = static_cast<unsigned long long>(-1);
        unsigned long long watcher_max = 0;
        void update_watcher_range(unsigned long long addr)
        {

            if (addr < watcher_min)
            {
                watcher_min = addr;
            }
            if (addr > watcher_max)
            {
                watcher_max = addr;
            }
        }
        unsigned long long get_range_size_watcher() const
        {
            return watcher_max - watcher_min;
        }

        unsigned long long clause_min = static_cast<unsigned long long>(-1);
        ;
        unsigned long long clause_max = 0;
        void update_clause_range(unsigned long long addr)
        {

            if (addr < clause_min)
            {
                clause_min = addr;
            }
            if (addr > clause_max)
            {
                clause_max = addr;
            }
        }
        unsigned long long get_range_size_clause() const
        {
            return clause_max - clause_min;
        }

        //read map :literal to times
        std::unordered_map<int, int> current_level_write_map;
        //write map :literal to times
        std::unordered_map<int, int> current_level_read_map;
        //hardware queues
        int current_level_read_old_times = 0;
        int current_level_write_same_times = 0;
        int current_level_write_conflict_times = 0;
        void sync_stats();
        unsigned long long total_read_old_times = 0;
        unsigned long long total_write_same_times = 0;
        unsigned long long total_write_conflict_times = 0;

        unsigned long long total_reads;
        unsigned long long total_writes;
    };

    ACC *create_acc(int watcher_proc_size,
                    int watcher_proc_num,
                    int clause_proc_num,
                    int miss_latency,
                    int watcher_process_latency,
                    int clause_process_latency,
                    int vault_memory_access_latency,
                    int cpu_to_vault_latency,
                    bool mode2,
                    int ctr_latency);
} // namespace MACC

#endif