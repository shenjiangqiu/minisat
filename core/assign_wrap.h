#ifndef ASSIGN_WRAP_H
#define ASSIGN_WRAP_H
#include <memory>
#include <tuple>
#include <map>
template <typename T, typename Time_t = int, typename Size_t = int, typename Clause_t = int>
class assign_wrap
{
public:
    ~assign_wrap()
    {
    }
    assign_wrap() = delete;
    void add_pushed_list(int index, T value) { pushed_other_list_items.insert(std::make_pair(index, value)); }
    void add_modified_list(int index, Clause_t value) { modified_clause_list_items.insert(std::make_pair(index, value)); }

    void set_watcher_size(Size_t size) { watcher_size = size; }
    Size_t get_watcher_size() const { return watcher_size; }
    const auto &get_pushed() const { return pushed_other_list_items; }
    const auto &get_modified() const { return modified_clause_list_items; }
    int get_level() const { return level; }
    void set_generated_conf(int conf) { generated_conf = conf; }
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
                                 generated_conf(-1),
                                 clause_size(0),
                                 level(level)
    {
    }
    assign_wrap(assign_wrap<T, Time_t, Size_t, Clause_t> &other) = default;
    assign_wrap(assign_wrap<T, Time_t, Size_t, Clause_t> &&other) = default;
    assign_wrap(const assign_wrap<T, Time_t, Size_t, Clause_t> &other) = default;
    const T &get_value() const { return value; }

private:
    T value;
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
        auto rt = std::make_shared<assign_wrap<T, Time_t, Size_t, Clause_t>>(assign_wrap<T, Time_t, Size_t, Clause_t>(value, watcher_size, depend_id, depend_value_weak, level));
        if (depend_id != -1)
        {
            //std::cout<<"added to the assignement"<<std::endl;
            depend_value->add_generated_assignments(depend_id, rt);
        }
        return rt;
    }
};

#endif