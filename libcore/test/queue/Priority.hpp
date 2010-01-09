#ifndef _SIRIKATA_QUEUE_BENCH_PRIORITY_HPP
#define _SIRIKATA_QUEUE_BENCH_PRIORITY_HPP
namespace Sirikata { namespace QueueBench {
class Message;
double standardfalloff (const UUID&a, const UUID&b);
extern std::tr1::function<double(const Vector3d&, const Vector3d&)> gLocationPriority;
extern std::tr1::function<double(const UUID&, const UUID&)> gPriority;
class MessagePriority{
    std::tr1::function<double(const UUID&, const UUID&)> mPriority;
public:
    MessagePriority(const std::tr1::function<double(const UUID&, const UUID&)> priority) {
        mPriority=priority;
    }
    bool operator() (const Message&a, const Message&b)const;
};
} }
#endif
