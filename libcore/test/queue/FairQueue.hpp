#ifndef _SIRIKATA_TEST_FAIR_QUEUE_HPP_
#define _SIRIKATA_TEST_FAIR_QUEUE_HPP_

#include <queue>
namespace Sirikata { namespace QueueBench {
template <class Message> class FairQueue {
    class MessagePriority {        
    public:
        Message msg;
        double outTime;
        double priority;
        MessagePriority(const Message&msg, double ttl, double priority) {
            this->msg=msg;
            this->outTime=ttl;
            this->priority=priority;
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
    std::deque<MessagePriority> mQueue;
    double mNow;
    bool mIsFair;
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
    void sort() {
        if (mIsFair) {
            std::sort_heap(mQueue.begin(),mQueue.end());        
        }else {
            std::sort(mQueue.begin(),mQueue.end());        
        }
    }
    typedef typename std::deque<MessagePriority>::iterator iterator;
    typedef typename std::deque<MessagePriority>::const_iterator const_iterator;
    iterator begin(){
        return mQueue.begin();
    }
    const_iterator begin()const{
        return mQueue.begin();
    }
    const_iterator end()const{
        return mQueue.end();
    }
    iterator end() {
        return mQueue.end();
    }
    FairQueue(bool isFair) {
        mNow=0;
        mIsFair=isFair;
    }
    bool nothingPopped(){
        return mNow==0;
    }
    void updatePriority(const Message&msg,size_t messageSize, double priority, bool onlyIfBetter=false){
        for (typename std::deque<MessagePriority>::iterator i=mQueue.begin(),ie=mQueue.end();i!=ie;++i) {
            if (i->msg==msg) {
                double newPriority=getPriority(messageSize,priority);
                if (i->outTime>newPriority||!onlyIfBetter) {
                    i->outTime=newPriority;
                    i->priority=priority;
                }
                if (mIsFair) {
                    std::make_heap(mQueue.begin(),mQueue.end());
                }
                return;
            }
        }
        push(msg,messageSize,priority);
    }
    void push(const Message& msg, size_t messageSize, double priority){
        double finish=getPriority(messageSize,priority);
        mQueue.push_back(MessagePriority(msg,finish,priority));
        if (mIsFair) {
            std::push_heap(mQueue.begin(),mQueue.end());
        }
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
        if (mIsFair) {
            std::pop_heap(mQueue.begin(),mQueue.end());
            mQueue.pop_back();
        }else {
            mQueue.pop_front();
        }
    }
};
} }
#endif
