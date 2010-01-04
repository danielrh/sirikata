#ifndef _SIRIKATA_TEST_MESSAGE_HPP_
#define _SIRIKATA_TEST_MESSAGE_HPP_

namespace Sirikata { namespace QueueBench {
class Message {
public:
    UUID dest;
    UUID source;
    int64 timeStamp;
    size_t size;
    bool operator<(const Message &other) const{
        return timeStamp<other.timeStamp;
    }
    bool operator>(const Message &other) const{
        return timeStamp>other.timeStamp;
    }

};
} }
#endif
