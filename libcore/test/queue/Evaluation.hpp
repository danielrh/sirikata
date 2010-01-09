
namespace Sirikata { namespace QueueBench {
typedef std::multimap<Message,int,MessagePriority> MessagePriorityMap;
void evaluateError(
    const std::vector<Message> &messages,
    const MessagePriorityMap&finalMessageOrder,
    const MessagePriorityMap&oracleMessageOrder,
    int numBins,
    bool binByPriority);
} }
