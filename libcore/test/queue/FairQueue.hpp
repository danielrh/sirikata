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
    std::vector<MessagePriority> mQueue;
    double mNow;
    double getPriority(size_t messageSize, double priority) {
        double finish=mNow+messageSize/priority;
        if (finish<mNow) {
            assert(0);
        }
        double eps=1.0e-30;
        while (!(finish>mNow)) {
            finish=mNow+eps;
            eps*=2;
        }
        return finish;
    }
  public:
    FairQueue() {
        mNow=0;
    }
    bool nothingPopped(){
        return mNow==0;
    }
    void updatePriority(const Message&msg,size_t messageSize, double priority, bool onlyIfBetter=false){
        for (typename std::vector<MessagePriority>::iterator i=mQueue.begin(),ie=mQueue.end();i!=ie;++i) {
            if (i->msg==msg) {
                double newPriority=getPriority(messageSize,priority);
                if (i->outTime>newPriority||!onlyIfBetter) {
                    i->outTime=newPriority;
                }
                std::make_heap(mQueue.begin(),mQueue.end());
                break;
            }
        }
    }
    void push(const Message& msg, size_t messageSize, double priority){
        double finish=getPriority(messageSize,priority);
        mQueue.push_back(MessagePriority(msg,finish));
        std::push_heap(mQueue.begin(),mQueue.end());
    }
    bool empty() const{
        return mQueue.empty();
    }
    size_t size() const{
        return mQueue.size();
    }

    const Message& front()const{
        return mQueue.front().msg;
    }
    double frontPriority()const{
        return mQueue.front().outTime;
    }
    void pop() {
        mNow=mQueue.front().outTime;
        std::pop_heap(mQueue.begin(),mQueue.end());
        mQueue.pop_back();
    }
};
} }
#endif
