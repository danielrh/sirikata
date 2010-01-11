#ifndef _SIRIKATA_TEST_ORACLE_OH_MESSAGE_QUEUE_HPP_
#define _SIRIKATA_TEST_ORACLE_OH_MESSAGE_QUEUE_HPP_

#include "FairQueue.hpp"
#include "Priority.hpp"
namespace Sirikata { namespace QueueBench {
class OracleOHMessageQueue{
    FairQueue<UUID> mObjectServicing;
    size_t mMaxObjectQueueSize;
    ObjectKnowledgeDescription mInputObjectHostKnowledge;
    ObjectKnowledgeDescription mFinalObjectHostKnowledge;
public:
    OracleOHMessageQueue(int maxObjectQueueSize):mObjectServicing(true/*fair*/) {
        mMaxObjectQueueSize=maxObjectQueueSize;
        mFinalObjectHostKnowledge.distanceKnowledge=false;
        mFinalObjectHostKnowledge.localRadiusKnowledge=ObjectKnowledgeDescription::NONE;
        mFinalObjectHostKnowledge.remoteRadiusKnowledge=ObjectKnowledgeDescription::FULL;


        mInputObjectHostKnowledge.distanceKnowledge=true;
        mInputObjectHostKnowledge.localRadiusKnowledge=ObjectKnowledgeDescription::FULL;
        mInputObjectHostKnowledge.remoteRadiusKnowledge=ObjectKnowledgeDescription::NONE;

        size_t i;
        size_t size=oSeg.mUUIDs.size();
        for(i=0;i<size;++i) {
            UUID uid=oSeg.mUUIDs[i];

            
        }
    }
    typedef std::tr1::unordered_map<UUID,FairQueue<Message>,UUID::Hasher> MessageMap;
    typedef std::tr1::unordered_map<UUID,std::deque<Message>, UUID::Hasher>OutputQueue;
    MessageMap mMessages;
    OutputQueue mPopFirst;
    void insertMessage(const Message&msg){
        double priority=gKnowledgePriority(msg.source,msg.dest,mInputObjectHostKnowledge);
        MessageMap::iterator where=mMessages.find(msg.dest);
        bool newMessage=(where==mMessages.end());
        if(newMessage) {
            ObjectData *od=oSeg[msg.dest];
            where=mMessages.find(msg.dest);
            if (where==mMessages.end()) {
                mMessages.insert(MessageMap::value_type(msg.dest,FairQueue<Message>(true)));
                where=mMessages.find(msg.dest);
            }
            where->second.push(msg,msg.size,priority);

        }else {
            if (where->second.empty()) {
                newMessage=true;
            }
            where->second.push(msg,msg.size,priority);
        }
        if (where->second.size()>=mMaxObjectQueueSize) {
            mPopFirst[msg.dest].push_back(where->second.front());
            where->second.pop();
        }
        if(newMessage) {
            mObjectServicing.push(msg.dest,1,gKnowledgePriority(msg.dest,msg.dest,mFinalObjectHostKnowledge));
        }
    }
    bool popMessage(const UUID &dest, Message&msg) {
        {
            OutputQueue::iterator where=mPopFirst.find(dest);
            if (where!=mPopFirst.end()) {
                if (!where->second.empty()) {
                    msg=where->second.front();
                    where->second.pop_front();
                    return true;
                }

            }
        }
        MessageMap::iterator where=mMessages.find(dest);
        if (where==mMessages.end()) {
            return false;
        }
        FairQueue<Message>* cur=&where->second;
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
                mObjectServicing.push(which,1,gKnowledgePriority(msg.dest,msg.dest,mFinalObjectHostKnowledge));
            }
        }
        return retval;
    }
    
};

} }

#endif
