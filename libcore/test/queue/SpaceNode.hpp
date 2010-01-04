#include "FairQueue.hpp"
#include "Message.hpp"
namespace Sirikata { namespace QueueBench {
class SpaceNode;
typedef std::tr1::unordered_map<UUID,SpaceNode*,UUID::Hasher> SpaceNodeMap;
class SpaceNode  {
    BoundingBox3d3f mBounds;
    UUID mName;
    typedef std::tr1::unordered_map<UUID,std::pair<double,double>, UUID::Hasher> PriorityMap;
    
    FairQueue<UUID> mNextMessage;
    PriorityMap mActivePriorities;
    const SpaceNodeMap*mKnownSpaceNodes;

    typedef std::tr1::unordered_map<UUID,FairQueue<Message>,UUID::Hasher > KeyMessageQueue;
    KeyMessageQueue mOHMessageQueue;
    KeyMessageQueue mSpaceMessageQueue;
    FairQueue<Message> mRNMessageQueue;
    std::tr1::unordered_set<UUID,UUID::Hasher> mActiveSpaces;
    FairQueue<UUID> mNearSpaces;
    bool mRNPrioritySet;
public:
    size_t waitingMessagesOH();
    size_t waitingMessagesSpace();
    size_t waitingMessagesRN();
    const UUID&id() {
        return mName;
    }
    SpaceNode(const BoundingBox3d3f & box);
    ///Either saved in the resource node or the global var--not responsible for allocation/deallocation
    void setKnownSpaceNodes(const SpaceNodeMap*);
    SpaceNode*split();
    Vector3d randomLocation()const;
    BoundingBox3d3f bounds() const{
        return mBounds;
    }
    


    ///pull off OH queues
    bool pullfromoh(Message&);
    void pullFromSpaces(size_t howMany);
    void pullFromOH(size_t howMany);
    bool pullbyoh(const UUID&, Message&);
    ///pulls by other space servers or RN's with UUID being space server node
    bool pull(const UUID&, Message&);
    void notifyNewSpaceMessage(const UUID&spaceId);
    void notifyNewOHMessage(const UUID&ohOrObj, double priority);
};
extern SpaceNodeMap gSpaceNodes;


} }
