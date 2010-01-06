#include "util/Standard.hh"
#include "util/UUID.hpp"
#include "OSeg.hpp"
#include "Random.hpp"
namespace Sirikata { namespace QueueBench {
OSeg oSeg;
UUID OSeg::random() {
    if (mSeg.empty())
        return UUID::null();
    typedef boost::uniform_int<> distribution_type;
    typedef boost::variate_generator<base_generator_type&, distribution_type> gen_type;
    gen_type gen(generator, distribution_type(0, mUUIDs.size()-1));
    return mUUIDs[gen()];
}

} }


