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
double standardfalloff (const UUID&a, const UUID&b) {
    ObjectData* ad=oSeg[a];
    ObjectData* bd=oSeg[b];
    
    return ad->radialSize*bd->radialSize*gLocationPriority(ad->location,bd->location);
}

std::tr1::function<double(const Vector3d&, const Vector3d&)> gLocationPriority(&oonnlgnlgn);
std::tr1::function<double(const UUID&, const UUID&)> gPriority(&standardfalloff);
} }
