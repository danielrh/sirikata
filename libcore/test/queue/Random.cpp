#include <util/Standard.hh>
#include <util/UUID.hpp>
#include "Random.hpp"
namespace Sirikata { namespace QueueBench {
base_generator_type generator(42u);

static uint64 uuidrange=0;
static uint64 uuidnumber=0;
void resetPseudorandomUUID(uint64 number) {
    uuidrange=number;
    uuidnumber=0;
}
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
        ++uuidnumber;
        data[0]=uuidnumber%256;
        data[1]=uuidnumber/256%256;
        data[2]=uuidnumber/256/256%256;
        data[3]=uuidnumber/256/256/256%256;
        data[4]=uuidnumber/256/256/256/256%256;
        data[5]=uuidnumber/256/256/256/256/256%256;
        data[6]=uuidnumber/256/256/256/256/256/256%256;
        data[7]=uuidnumber/256/256/256/256/256/256/256%256;

        data[8]=uuidrange%256;
        data[9]=uuidrange/256%256;
        data[10]=uuidrange/256/256%256;
        data[11]=uuidrange/256/256/256%256;
        data[12]=uuidrange/256/256/256/256%256;
        data[13]=uuidrange/256/256/256/256/256%256;
        data[14]=uuidrange/256/256/256/256/256/256%256;
        data[15]=uuidrange/256/256/256/256/256/256/256%256;
    }
    return UUID(data,sizeof(UUID));
}
} }
