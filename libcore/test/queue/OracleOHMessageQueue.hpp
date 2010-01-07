#ifndef _SIRIKATA_TEST_ORACLE_OH_MESSAGE_QUEUE_HPP_
#define _SIRIKATA_TEST_ORACLE_OH_MESSAGE_QUEUE_HPP_

#include "FairQueue.hpp"
#include "Priority.hpp"
namespace Sirikata { namespace QueueBench {
class OracleOHMessageQueue{
    FairQueue<UUID> mObjectServicing;
public:
    OracleOHMessageQueue() {
        size_t i;
        size_t size=oSeg.mUUIDs.size();
        for(i=0;i<size;++i) {
            UUID uid=oSeg.mUUIDs[i];

            
        }
    }
    typedef std::tr1::unordered_map<UUID,FairQueue<Message>,UUID::Hasher> MessageMap;
    MessageMap mMessages;
    void insertMessage(const Message&msg){
        double priority=standardfalloff(msg.source,msg.dest);
        MessageMap::iterator where=mMessages.find(msg.dest);
        bool newMessage=(where==mMessages.end());
        if(newMessage) {
            ObjectData *od=oSeg[msg.dest];
            mMessages[msg.dest].push(msg,msg.size,priority);
        }else {
            if (where->second.empty()) {
                newMessage=true;
            }
            where->second.push(msg,msg.size,priority);
        }
        if(newMessage) {
            mObjectServicing.push(msg.dest,1,standardfalloff(msg.dest,msg.dest));
        }
    }
    bool popMessage(const UUID &dest, Message&msg) {
        FairQueue<Message>* cur=&mMessages[dest];
        if (cur->empty())
            return false;
        msg=cur->front();
        cur->pop();
        return true;
    }
    
    bool popMessage(Message&msg) {
        size_t num_objects_to_serve=mObjectServicing.size();
        bool retval=false;
        for (size_t i=0;i<num_objects_to_serve&&!retval;++i) {
            UUID which=mObjectServicing.front();
            mObjectServicing.pop();
            retval= popMessage(which,msg);
            if (retval) {
                mObjectServicing.push(which,1,standardfalloff(msg.dest,msg.dest));
            }
        }
        return retval;
    }
    
};

} }

#endif
