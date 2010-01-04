#ifndef _SIRIKATA_TEST_ORACLE_MESSAGE_QUEUE_HPP_
#define _SIRIKATA_TEST_ORACLE_MESSAGE_QUEUE_HPP_

#include "FairQueue.hpp"
#include "Priority.hpp"
namespace Sirikata { namespace QueueBench {
class OracleMessageQueue{
public:
    OracleMessageQueue() {
    }
    FairQueue<Message> mMessages;
    void insertMessage(const Message&msg){
        mMessages.push(msg,msg.size,gPriority(msg.source,msg.dest));
        assert(mMessages.nothingPopped());
    }
    bool popMessage(Message&msg) {
        bool retval= !mMessages.empty();
        if( retval) {
            msg=mMessages.front();
            mMessages.pop();
            return true;
        }
        return false;
    }
    
};

} }

#endif
