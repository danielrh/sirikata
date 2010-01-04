#include <util/Standard.hh>
#include <util/UUID.hpp>
#include "Message.hpp"
#include "Generator.hpp"
#include "OSeg.hpp"
namespace Sirikata { namespace QueueBench {
bool gNoLatePush=true;
RandomMessageGenerator::RandomMessageGenerator() {
    mTimeStamp=0;
}
Message RandomMessageGenerator::generate(const UUID&source) {
    UUID dest=oSeg.random();
    Message retval;
    retval.dest=dest;
    retval.source=source;
    retval.size=1;
    retval.timeStamp=mTimeStamp++;
    return retval;
}

} }


