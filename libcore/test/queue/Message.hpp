#ifndef _SIRIKATA_TEST_MESSAGE_HPP_
#define _SIRIKATA_TEST_MESSAGE_HPP_

namespace Sirikata { namespace QueueBench {
class Message {
public:
    UUID dest;
    UUID source;
    int64 uid;
    size_t size;
    bool operator<(const Message &other) const{
        return uid<other.uid;
    }
    bool operator>(const Message &other) const{
        return uid>other.uid;
    }

};
} }
#endif
