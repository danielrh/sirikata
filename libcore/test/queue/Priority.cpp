#include "util/Standard.hh"
#include "util/UUID.hpp"
#include "Priority.hpp"
#include "OSeg.hpp"
#include <cmath>
namespace Sirikata { namespace QueueBench {
double oonnlgnlgn(const Vector3d &a, const Vector3d b) {
    double distance=(a-b).length();
    if (distance<2) distance=2;
    double lgdistance=std::log(distance);
    double dlgd=distance*lgdistance/10000;
//    std::cout<<'('<<ad->radialSize<<'*'<<bd->radialSize<<'/'<<dlgd<<'2'<<'\n';

    return 1./(dlgd*dlgd);
}
double roughStandardfalloff (const UUID&a, const UUID&b) {
    ObjectData* ad=oSeg[a];
    ObjectData* bd=oSeg[b];
    int exp;
    frexp(bd->radialSize,&exp);
    double bdrsize=ldexp(1.0,exp);
    frexp(ad->radialSize,&exp);
    double adrsize=ldexp(1.0,exp);
//    adrsize=bdrsize=1.0;
    return adrsize*bdrsize*gLocationPriority(ad->location,bd->location);
}


double standardfalloff (const UUID&a, const UUID&b) {
    ObjectData* ad=oSeg[a];
    ObjectData* bd=oSeg[b];
    
    return ad->radialSize*bd->radialSize*gLocationPriority(ad->location,bd->location);
}

double randomLocationPriority(const Vector3d &a, const Vector3d b) {
    size_t c=std::tr1::hash<float>()(a.x);
    size_t d=std::tr1::hash<float>()(a.y);
    size_t e=std::tr1::hash<float>()(a.z);
    size_t f=std::tr1::hash<float>()(b.x);
    size_t g=std::tr1::hash<float>()(b.y);
    size_t h=std::tr1::hash<float>()(b.z);
    size_t j=c^d^e^f^g^h;
    j%=2147483645;
    j+=1;
    return j/(2147483645.);
}
double randomPriority(const UUID&a, const UUID&b) {
    ObjectData* ad=oSeg[a];
    ObjectData* bd=oSeg[b];
    return randomLocationPriority(ad->location,bd->location);
}
#if 1
std::tr1::function<double(const Vector3d&, const Vector3d&)> gLocationPriority(&oonnlgnlgn);
std::tr1::function<double(const UUID&, const UUID&)> gPriority(&roughStandardfalloff);
#else
std::tr1::function<double(const Vector3d&, const Vector3d&)> gLocationPriority(&randomLocationPriority);
std::tr1::function<double(const UUID&, const UUID&)> gPriority(&randomPriority);
#endif
} }
