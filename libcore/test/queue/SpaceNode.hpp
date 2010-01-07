#include "FairQueue.hpp"
#include "Message.hpp"
namespace Sirikata { namespace QueueBench {
class SpaceNode;
typedef std::tr1::unordered_map<UUID,SpaceNode*,UUID::Hasher> SpaceNodeMap;
class SpaceNode  {
    SpaceNode*mParent;
    BoundingBox3d3f mBounds;
    UUID mName;
    typedef std::tr1::unordered_map<UUID,std::pair<double,double>, UUID::Hasher> PriorityMap;
    FairQueue<UUID> mNextChild;
    FairQueue<UUID> mNextMessage;
    PriorityMap mActivePriorities;
    SpaceNodeMap*mKnownSpaceNodes;

    typedef std::tr1::unordered_map<UUID,FairQueue<Message>,UUID::Hasher > KeyMessageQueue;
    KeyMessageQueue mOHMessageQueue;
    KeyMessageQueue mSpaceMessageQueue;
    KeyMessageQueue mRNMessageQueue;
    KeyMessageQueue mChildMessageQueue;
    FairQueue<Message> mParentMessageQueue;
    std::tr1::unordered_set<UUID,UUID::Hasher> mActiveSpaces;
    std::tr1::unordered_set<UUID,UUID::Hasher> mActiveRNs;
    FairQueue<UUID> mNearSpaces;
    FairQueue<UUID> mNearRNs;
    bool mRNPrioritySet;
    double mDefaultParentPriority;
public:
    size_t waitingMessagesOH();
    size_t waitingMessagesSpace();
    size_t waitingMessagesChild();
    size_t waitingMessagesParent();
    size_t waitingMessagesRN();
    const UUID&id() {
        return mName;
    }
    SpaceNode(const BoundingBox3d3f & box, SpaceNode* parent);
    ///Either saved in the resource node or the global var--not responsible for allocation/deallocation

    SpaceNode*split();
    Vector3d randomLocation()const;
    BoundingBox3d3f bounds() const{
        return mBounds;
    }
    
    bool hasParent()const {
        return mParent!=NULL;
    }
    
    ///pull off OH queues
    bool pullfromoh(Message&);
    void pullFromSpaces(size_t howMany);
    void pullFromRNs(size_t howMany);
    void pullFromOH(size_t howMany);
    bool pullbyoh(const UUID&, Message&);
    bool pullbyrn(const UUID&, Message&);
    bool pullbyparent(Message&);
    bool pullbychild(const UUID&, Message&);
    ///pulls by other space servers or RN's with UUID being space server node
    bool pull(const UUID&, Message&);
    void notifyNewParentMessage(const UUID&spaceId,double priority);
    void notifyNewSpaceMessage(const UUID&spaceId);
    void notifyNewRNMessage(const UUID&spaceId);
    void notifyNewOHMessage(const UUID&ohOrObj, double priority);
};
extern SpaceNodeMap gSpaceNodes;


} }
