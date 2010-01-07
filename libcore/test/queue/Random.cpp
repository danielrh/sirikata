#include <util/Standard.hh>
#include <util/UUID.hpp>
#include "Random.hpp"
namespace Sirikata { namespace QueueBench {
base_generator_type generator(42u);
UUID pseudorandomUUID() {
    typedef boost::uniform_int<> distribution_type;
    typedef boost::variate_generator<base_generator_type&, distribution_type> gen_type;
    gen_type gen(generator, distribution_type(0, 255));
    
    unsigned char data[sizeof(UUID)]={0};
    if (0) {
        for (int i=0;i<(int)sizeof(UUID);++i) {
            data[i]=gen();
        }
    }else {
        static uint64 i=0;
        ++i;
        data[0]=i%256;
        data[1]=i/256%256;
        data[2]=i/256/256%256;
        data[3]=i/256/256/256%256;
        data[4]=i/256/256/256/256%256;
        data[5]=i/256/256/256/256/256%256;
        data[6]=i/256/256/256/256/256/256%256;
        data[7]=i/256/256/256/256/256/256/256%256;
    }
    return UUID(data,sizeof(UUID));
}
} }
