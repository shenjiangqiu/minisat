#ifndef ACC_H
#define ACC_H
#include <math.h>
#include <algorithm>
#include <signal.h>
#include <unistd.h>

#include <memory>
class process_unit
{
public:
    bool is_ready()
    {
        return !m_busy;
    }
    int ready_time()
    {
        return m_ready_time;
    }
    void dispatch(int current_time)
    {
        m_busy = true;
        m_ready_time = current_time + process_time;
    }

private:
    const int process_time;
    bool m_busy;
    int m_ready_time;
};

class Time_record
{
public:
    Time_record(int start_time_watch_size,
                int start_time_clause_size) : start_time_watch(start_time_watch_size, 0),
                                              start_time_clause(start_time_clause_size, 0) {}
    int get_start_time_watch(int index)
    {
        return start_time_watch[index];
    }
    int get_start_time_clause(int index)
    {
        return start_time_clause[index];
    }
    int get_watch_size() { return start_time_watch.size(); }
    int get_clause_size() { return start_time_clause.size(); }
    void set_start_time_watch(int t_start_time_watch, int index) { start_time_watch[index] = t_start_time_watch; }
    void set_start_time_clause(int t_start_time_clause, int index) { start_time_clause[index] = t_start_time_clause; }

private:
    std::vector<int> start_time_watch;
    std::vector<int> start_time_clause;
};

template <typename T>
class ACC
{
public:
    ACC() = default;
    ACC(int watcher_proc_size,
        int watcher_proc_num,
        int clause_proc_size,
        int clause_proc_num) : w_size(watcher_proc_size),
                               w_num(watcher_proc_num),
                               c_hash_size(clause_proc_size),
                               c_num(clause_proc_num),
                               m_using_watcher_unit(0),
                               m_using_clause_unit(0),
                               m_ready(false) {}
    void print_on()
    {
        print_log = true;
    }
    void print_off()
    {
        print_log = false;
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
        m_ready = false;
        value_queue.clear();
        time_records.clear();
    }

    int start_sim()
    {
        for (auto &&assignment : value_queue)
        {
        }
    }
    void push_to_trail(std::shared_ptr<T> value)
    {
        value_queue.push_back(value);
    }
    int get_trail_size() const
    {
        return value_queue.size();
    }

    bool have_watcher_unit_ready() const { return m_using_watcher_unit < w_num; }
    bool have_clause_unit_ready() const { return m_using_watcher_unit < c_num; }
    void value_push(std::shared_ptr<T> value) { value_queue.push_back(value); }
    int get_max_level() { return (*std::prev(value_queue.cend()))->get_level(); }
    void push_time_records(std::shared_ptr<Time_record> time) { time_records.push_back(time); }

private:
    int w_size;
    int w_num;
    int c_num;
    int c_hash_size;
    bool m_ready;
    std::vector<std::shared_ptr<T>> value_queue;
    std::vector<std::shared_ptr<Time_record>> time_records;
    int m_using_watcher_unit;
    int m_using_clause_unit;
    bool print_log;
};

template <typename T>
std::shared_ptr<ACC<T>> create_acc()
{
    return std::make_shared<ACC<T>>();
}

template <typename V>
class task
{
public:
    V value;

    int start;
    int end;
    int remaining;
    int total;
    bool is_first;
};
template <typename V>
class unit
{
public:
    void push_task(task<V> &task,int time)
    {  
        task.start = time;
        task.end = time+ task.is_first ?cache_line_hit_latency:cache_line_miss_latency;
        next_run_time=task.end<next_run_time ? task.end : next_run_time;
        
        task_queue.push(task);
        
        

    }

    void run(int time)
    {
        auto &task=task_queue.front();
        auto &value=task.value;
        auto &generated=
    }
    bool need_run(int time)
    {
        return next_run_time <= time;
    }
    int get_next_run_time() { return next_run_time; }
    int get_remaining() { return remaining; }
    int get_working() { return working; }

private:
    int next_run_time;
    std::queue<task<V>> task_queue;
    int total_size;
    int remaining;
    int working;
    int cache_line_hit_latency;
    int cache_line_miss_latency; 
};
template <typename T>
class watcher_list_process_unit
{

public:
    void run(int time)
    {
        for (auto &&unit : m_units)
        {
            if (unit.need_run(time))
            {
                unit.run(time);
            }
        }
    }
    bool need_run(int time)
    {
        return next_run_time <= time;
    }
    void push_task(const task<T> &mtask)
    {
        mtasks.push(mtask);
    }

private:
    std::queue<std::shared_ptr<task<T>>> mtasks;
    int next_run_time;
    int total_process_size;
    int remaining_unit;
    std::vector<unit> m_units;
};

#endif