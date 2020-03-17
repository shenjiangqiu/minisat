#ifndef _EVENT_H_
#define _EVENT_H_

#include <memory>
#include <iostream>
#include <queue>

template <typename EventType, typename EventComp>
class EventQueue
{
public:
    EventQueue(const EventComp &comp) : m_event_queue(comp) {}
    void push(const EventType &event) { m_event_queue.push(event); }
    const EventType &top() const { return m_event_queue.top(); }
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
    void clear() { m_event_queue.clear(); }

private:
    std::priority_queue<EventType, std::deque<EventType>, EventComp> m_event_queue;
};
enum class EventType
{
    ReadWatcherList,
    ProcessWatcherList,
    FinishAndSendClause,
    ProcessClause,

};
std::ostream &operator<<(std::ostream &os, EventType type)
{
    switch (type)
    {
    case EventType::ReadWatcherList:
        os << "ReadWatcherList";
        break;
    case EventType::ProcessWatcherList:
        os << "ProcessWatcherList";
        break;
    case EventType::FinishAndSendClause:
        os << "FinishAndSendClause";
        break;
    case EventType::ProcessClause:
        os << "ProcessClause";
        break;
    }
    os << std::endl;
    return os;
}
enum class HardwareType
{
    watcherListUnit,
    ClauseUnit
};
template <typename T>
struct EventValue
{
    EventValue(EventType _tp, int _id, int _sz, std::weak_ptr<T> v, HardwareType ht, int hi) : type(_tp), index(_id), size(_sz), value(v), hType(ht), HardwareId(hi) {}
    EventType type;
    int index;
    int size;
    std::weak_ptr<T> value; // the assignment wraper
    HardwareType hType;
    int HardwareId;
};

template <typename T>
class Event
{
public:
    Event(const EventValue<T> &value, int startTime, int endTime) : start_time(startTime), end_time(endTime), mEventValue(value) {}
    int get_key() const { return end_time; }
    const EventValue<T> &get_value() const { return mEventValue; }
    int get_start_time() const { return start_time; }
    int get_end_time() const { return end_time; }

private:
    int start_time;
    int end_time;
    EventValue<T> mEventValue;
};

template <typename T>
std::ostream &operator<<(std::ostream &os, const EventValue<T> &eventValue)
{
    os << "EventValue:type: " << eventValue.type << std::endl;
    os << "EventValue:value: " << eventValue.value.lock()->get_value() << std::endl;
    os << "EventValue:index: " << eventValue.index << std::endl;
    os << "EventValue:size: " << eventValue.size << std::endl;
    return os;
}
template <typename T>
std::ostream &operator<<(std::ostream &os, const Event<T> &event)
{
    os << "Event start time: " << event.get_start_time() << std::endl;
    os << "Event end time: " << event.get_end_time() << std::endl;
    os << event.get_value() << std::endl;
    return os;
}
template <typename T>
auto EventComp = [](const Event<T> &e1, const Event<T> &e2) {
    return e1.get_key() > e2.get_key();
};

#endif /* _EVENT_H_ */