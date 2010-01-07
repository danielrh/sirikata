#ifndef _SIRIKATA_TEST_OBJECT_HOST_HPP_
#define _SIRIKATA_TEST_OBJECT_HOST_HPP_
#include "FairQueue.hpp"
#include "Message.hpp"
namespace Sirikata { namespace QueueBench {
class ObjectHost{
    double mCurrentPriority;
    std::tr1::unordered_map<UUID,FairQueue<Message>,UUID::Hasher > mObjectMessageOrder;
    UUID mName;
    bool mStreamPerObject;
    bool mDistanceKnowledge;
    bool mRemoteObjectRadiusKnowledge;
    bool mLocalObjectRadiusKnowledge;
    size_t mRestoredPullOrder;
    typedef std::tr1::unordered_set<UUID,UUID::Hasher> ObjectSet;
    ObjectSet mObjects;

    ///objects without messages
    ObjectSet mDefunctObjects;
    ///UUID can either be SpaceNode id or it can be object ID depending on whether space node has OID info
    FairQueue<UUID> mPullOrder;
    ///Map from space node to the aggregate priority of all objects connected there
    typedef std::tr1::unordered_map<UUID, double, UUID::Hasher> SpaceNodePriorityMap;
    SpaceNodePriorityMap mSpaceNodePriority;
    double ohMessagePriority(const Message&msg);
    void restorePullOrder();
    void checkBookkeeping();
public:
    size_t objectMessageQueueSize()const;
    bool queuePerObject() const{
        return mStreamPerObject;
    }
    const UUID &id() {
        return mName;
    }
    
    void insertMessage (const Message&msg);
    template <class Container> void restorePullOrder(const Container &cont);
    ///If streamPerObject false then it assumes a stream per spacenode
    ObjectHost(bool streamPerObject, bool distanceKnowledge,  bool remoteRadiusKnowledge, bool localRadiusKnowledge=true);
    void addObject(const UUID&);
    bool pull(const UUID &, Message&);
    bool pull(Message&);

    bool pullbyspace(const UUID&,Message&);
    bool getPriority(const UUID&,double&);
    void notifyNewSpaceMessage(const UUID&ohOrObj);
};
extern std::tr1::unordered_map<UUID,ObjectHost*,UUID::Hasher>gObjectHosts;
} }
#endif
