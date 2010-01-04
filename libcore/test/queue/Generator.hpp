#include "Message.hpp"
namespace Sirikata { namespace QueueBench {
class Generator{
public:
    virtual Message generate(const UUID & objectID)=0;

    virtual ~Generator(){}
};
class RandomMessageGenerator:public Generator{
    int64 mTimeStamp;
public:
    RandomMessageGenerator();
    virtual Message generate(const UUID & objectID);
};

extern bool gNoLatePush;

} }
