#ifndef _SIRIKATA_TEST_FAIR_QUEUE_HPP_
#define _SIRIKATA_TEST_FAIR_QUEUE_HPP_

#include <queue>
namespace Sirikata { namespace QueueBench {
template <class Message> class FairQueue {
    class MessagePriority {        
    public:
        Message msg;
        double outTime;
        MessagePriority(const Message&msg, double ttl) {
            this->msg=msg;
            this->outTime=ttl;
        }
        bool operator<(const MessagePriority &other) const{
            if (!(outTime>other.outTime)) {
                if (!(other.outTime>outTime)) {
                    return msg>other.msg;
                }
            }
            return outTime>other.outTime;
        }
    };
    std::priority_queue<MessagePriority> mQueue;
    double mNow;
  public:
    FairQueue() {
        mNow=0;
    }
    bool nothingPopped(){
        return mNow==0;
    }
    void updatePriority(const Message&msg,size_t messageSize, double priority){
        //NOT_IMPLEMENTED();
    }
    void push(const Message& msg, size_t messageSize, double priority){
        double finish=mNow+messageSize/priority;
        if (finish<mNow) {
            assert(0);
        }
        double eps=1.0e-30;
        while (!(finish>mNow)) {
            finish=mNow+eps;
            eps*=2;
        }
        mQueue.push(MessagePriority(msg,finish));
    }
    bool empty() const{
        return mQueue.empty();
    }
    size_t size() const{
        return mQueue.size();
    }

    const Message& front()const{
        return mQueue.top().msg;
    }
    double frontPriority()const{
        return mQueue.top().outTime;
    }
    void pop() {
        mNow=mQueue.top().outTime;
        mQueue.pop();
    }
};
} }
#endif
