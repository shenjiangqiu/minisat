#ifndef _EVENT_QUEUE_H
#define _EVENT_QUEUE_H
#include <queue>

template <typename EventType, typename EventComp>
class EventQueue
{
public:
    EventQueue(const EventComp &comp) : m_event_queue(comp) {}
    void push(const EventType &event) { m_event_queue.push(event); }
    const EventType &top() const { return m_event_queue.top(); }
    void pop() { m_event_queue.pop(); }
    auto get_next_time() const
    {
        return m_event_queue.top().get_key();
    }
    auto get_next_event() const
    {
        return m_event_queue.top().get_value();
    }

private:
    std::priority_queue<EventType, std::deque<EventType>, EventComp> m_event_queue;
};

#endif /* _EVENT_QUEUE_H */