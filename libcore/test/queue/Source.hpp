namespace Sirikata { namespace QueueBench {
class Source {
public:
    virtual  ~Source(){}
    bool pull (Message&)=0;
    bool pull (const UUID&, Message&)=0;
};

} }
