#include <util/Standard.hh>
#include <util/UUID.hpp>
#include "Random.hpp"
namespace Sirikata { namespace QueueBench {
base_generator_type generator(42u);
UUID pseudorandomUUID() {
    typedef boost::uniform_int<> distribution_type;
    typedef boost::variate_generator<base_generator_type&, distribution_type> gen_type;
    gen_type gen(generator, distribution_type(0, 255));
    
    unsigned char data[sizeof(UUID)];
    for (int i=0;i<sizeof(UUID);++i) {
        data[i]=gen();
    }
    return UUID(data,sizeof(UUID));
}
} }
