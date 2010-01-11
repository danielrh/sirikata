#ifndef _SIRIKATA_QUEUE_BENCH_PRIORITY_HPP
#define _SIRIKATA_QUEUE_BENCH_PRIORITY_HPP
namespace Sirikata { namespace QueueBench {
struct ObjectKnowledgeDescription {
    enum Fuzzy {
        NONE=0,
        FUZZY=-1,
        FULL=1
    };
    Fuzzy localRadiusKnowledge;
    Fuzzy remoteRadiusKnowledge;
    bool distanceKnowledge;
};

class Message;
double standardfalloff (const UUID&a, const UUID&b);
extern std::tr1::function<double(const Vector3d&, const Vector3d&)> gLocationPriority;
extern std::tr1::function<double(const UUID&, const UUID&)> gPriority;
extern std::tr1::function<double(const UUID&, const UUID&, const ObjectKnowledgeDescription&)> gKnowledgePriority;
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
