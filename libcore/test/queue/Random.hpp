#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
namespace Sirikata { namespace QueueBench {
//typedef boost::ecuyer1988 base_generator_type//
typedef boost::minstd_rand base_generator_type;
extern base_generator_type generator;
Sirikata::UUID pseudorandomUUID();
} }
