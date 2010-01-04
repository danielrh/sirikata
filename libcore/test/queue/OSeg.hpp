#ifndef _SIRIKATA_TEST_OSEG_HPP_
#define _SIRIKATA_TEST_OSEG_HPP_

#include "ObjectHost.hpp"
namespace Sirikata { namespace QueueBench {
class ObjectData {
public:
    UUID spaceServerNode;
    UUID objectHost;
    double radialSize;//could be baked into UUID
    Vector3d location;//doesn't belong here, but useful for 'baseline'
};

class OSeg {
    typedef std::tr1::unordered_map<UUID,ObjectData,UUID::Hasher> OSegMap;
    OSegMap mSeg;
public:
    std::vector<UUID> mUUIDs;
    ObjectData*operator[](const UUID&data) {
        OSegMap::iterator where=mSeg.find(data);
        if (where==mSeg.end())
            return NULL;
        return &where->second;
    }
    const ObjectData*operator[](const UUID&data) const{
        OSegMap::const_iterator where=mSeg.find(data);
        if (where==mSeg.end())
            return NULL;
        return &where->second;
    }
    ObjectData* insert(const UUID &oid, const ObjectData&data){
        mUUIDs.push_back(oid);
        OSegMap::iterator where=mSeg.insert(OSegMap::value_type(oid,data)).first;
        gObjectHosts[data.objectHost]->addObject(oid);
        return &where->second;
    }
    UUID random() {
        if (mSeg.empty())
            return UUID::null();
        return mUUIDs[rand()%mUUIDs.size()];
    }
};
extern OSeg oSeg;
} }
#endif
