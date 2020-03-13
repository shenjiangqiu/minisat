#ifndef ASSIGN_WRAP_H
#define ASSIGN_WRAP_H
#include <memory>
#include <tuple>
template <typename T, typename Time_t = int, typename Size_t = int, typename Clause_t = int>
class assign_wrap
{
public:
    assign_wrap() = delete;

    Time_t get_time() const { return end_time - start_time; }
    void add_pushed_list(int index, T value) { pushed_other_list_items.insert(std::make_pair(index, value)); }
    void add_modified_list(int index, Clause_t value) { modified_clause_list_items.insert(std::make_pair(index, value)); }
    void setstart_time(Time_t time) { start_time = time; }
    void setend_time(Time_t time) { end_time = time; }
    Time_t get_start_time() const { return start_time; }
    Time_t get_end_time() const { return end_time; }
    void set_watcher_size(Size_t size) { watcher_size = size; }
    Size_t get_watcher_size() const { return watcher_size; }
    const auto &get_pushed() const { return pushed_other_list_items; }
    const auto &get_modified() const { return modified_clause_list_items; }
    int get_level() const { return level; }
    int get_generated_conf() const { return generated_conf; }
    const auto &get_generated_assignments() const { return generated_assignments; }
    void add_generated_assignments(int index, std::weak_ptr<assign_wrap<T, Time_t, Size_t, Clause_t>> tgenerated)
    {
        generated_assignments.insert(std::make_pair(index, tgenerated));
    }
    friend class assign_wrap_factory;

    assign_wrap(T value,
                Size_t watcher_size,
                int depend_id,
                std::weak_ptr<assign_wrap<T, Time_t, Size_t>> depend_value,
                int level = 0) : value(value),
                                 watcher_size(watcher_size),
                                 depend_id(depend_id),
                                 depend_value(depend_value),
                                 level(level),
                                 clause_size(0)
    {
    }
    assign_wrap(assign_wrap<T, Time_t, Size_t, Clause_t> &other) = default;
    assign_wrap(assign_wrap<T, Time_t, Size_t, Clause_t> &&other) = default;
    assign_wrap(const assign_wrap<T, Time_t, Size_t, Clause_t> &other) = default;

private:
    T value;
    Time_t start_time;
    Time_t end_time;
    Time_t wather_list_finish_time;
    Time_t clause_finish_time;
    Size_t watcher_size;
    int depend_id;
    std::weak_ptr<assign_wrap<T, Time_t, Size_t, Clause_t>> depend_value;
    //std::pair<int, std::weak_ptr<assign_wrap<T, Time_t, Size_t, Clause_t>>> Depends;
    std::map<int, Clause_t> modified_clause_list_items;
    std::map<int, T> pushed_other_list_items;
    int generated_conf;

    std::map<int, std::weak_ptr<assign_wrap<T, Time_t, Size_t, Clause_t>>> generated_assignments; // must make the ptr weak here to break circular;

    Size_t clause_size;

    int level;
};
template <typename SharedPointType>
struct GetType
{
    typedef typename SharedPointType::element_type type;
};
class assign_wrap_factory
{
public:
    template <typename T, typename Time_t, typename Size_t, typename Clause_t = int>
    std::shared_ptr<assign_wrap<T, Time_t, Size_t, Clause_t>> create(T value,
                                                                     Size_t watcher_size,
                                                                     int depend_id,
                                                                     std::shared_ptr<assign_wrap<T, Time_t, Size_t, Clause_t>> depend_value,
                                                                     int level = 0)
    {
        auto depend_value_weak = std::weak_ptr<assign_wrap<T, Time_t, Size_t, Clause_t>>(depend_value);
        auto rt=std::make_shared<assign_wrap<T, Time_t, Size_t, Clause_t>>(assign_wrap<T, Time_t, Size_t, Clause_t>(value, watcher_size, depend_id, depend_value_weak, level));
        if(depend_id!=-1){
            //std::cout<<"added to the assignement"<<std::endl;
            depend_value->add_generated_assignments(depend_id,rt);
        }
        return rt;
    }
};

#endif