#include "util/Standard.hh"
#include "util/UUID.hpp"
#include "SpaceNode.hpp"
#include "ObjectHost.hpp"
#include "OSeg.hpp"
#include "Priority.hpp"
#include "Random.hpp"
namespace Sirikata{ namespace QueueBench {
std::tr1::unordered_map<UUID,SpaceNode*,UUID::Hasher>gSpaceNodes;
SpaceNode::SpaceNode(const BoundingBox3d3f & box,SpaceNode*parent, bool resort_message_queues):mBounds(box),mName(pseudorandomUUID()),mNextChild(true),mNextMessage(true),mParentMessageQueue(resort_message_queues),mNearSpaces(true),mNearRNs(true){
    mResortMessageQueues=resort_message_queues;
    gSpaceNodes[id()]=this;
    mParent=parent;
    if (mParent) {
        mKnownSpaceNodes=parent->mKnownSpaceNodes;
        assert((*mParent->mKnownSpaceNodes)[id()]!=this);
        (*mParent->mKnownSpaceNodes)[id()]=this;//FIXME this only works for one level of RN heirharchy which is probably all we'll ever be able to simulate with this pile o' bolts
    }else {
        mKnownSpaceNodes=new SpaceNodeMap;
    }
    mRNPrioritySet=false;
}
SpaceNode*SpaceNode::split(){
    typedef boost::uniform_int<> distribution_type;
    typedef boost::variate_generator<base_generator_type&, distribution_type> gen_type;
    gen_type gen(generator, distribution_type(0, 1/*2?!*/));

    int axis=gen();
    BoundingBox3d3f oldbounds;
    BoundingBox3d3f newbounds;
    boost::uniform_real<> uni_dist(0,1);
    boost::variate_generator<base_generator_type&, boost::uniform_real<> > uni(generator, uni_dist);

    double splitline=uni();
    if (axis==0) {
        oldbounds=BoundingBox3d3f(mBounds.min(),
                                  mBounds.min()+Vector3d(mBounds.diag().x*splitline,mBounds.diag().y,mBounds.diag().z));
        newbounds=BoundingBox3d3f(mBounds.min()+Vector3d(mBounds.diag().x*splitline,0.,0.),
                                  mBounds.max());
    }else if (axis==2) {
        oldbounds=BoundingBox3d3f(mBounds.min(),
                                  mBounds.min()+Vector3d(mBounds.diag().x,mBounds.diag().y,mBounds.diag().z*splitline));
        newbounds=BoundingBox3d3f(mBounds.min()+Vector3d(0.,0.,mBounds.diag().z*splitline),
                                  mBounds.max());
        
    }else {
        oldbounds=BoundingBox3d3f(mBounds.min(),
                                  mBounds.min()+Vector3d(mBounds.diag().x,mBounds.diag().y*splitline,mBounds.diag().z));
        newbounds=BoundingBox3d3f(mBounds.min()+Vector3d(0.,mBounds.diag().y*splitline,0.),
                                  mBounds.max());
    }
    mBounds=oldbounds;
    SpaceNode *retval=new SpaceNode(newbounds,mParent,mResortMessageQueues);
    if (mParent) {
        (*mParent->mKnownSpaceNodes)[retval->id()]=this;
    }
    return retval;
}

Vector3d SpaceNode::randomLocation() const{
    boost::uniform_real<> uni_dist(0,1);
    boost::variate_generator<base_generator_type&, boost::uniform_real<> > uni(generator, uni_dist);
    double xm=uni();
    double ym=uni();
    double zm=uni();
    return mBounds.min()+Vector3d(mBounds.diag().x*xm,
                                  mBounds.diag().y*ym,
                                  mBounds.diag().z*zm);
                                  
}
FairQueue<Message>&SpaceNode::getQueueByUUID(KeyMessageQueue&q, const UUID&uuid){
    KeyMessageQueue::iterator where=q.find(uuid);
    if (where==q.end()) {
        q.insert(KeyMessageQueue::value_type(uuid,FairQueue<Message>(mResortMessageQueues)));
        where=q.find(uuid);
    }
    return where->second;

}
void SpaceNode::notifyNewOHMessage(const UUID&id, double priority) {
    PriorityMap::iterator where=mActivePriorities.find(id);
    if (where!=mActivePriorities.end()) {
        if (where->second.second<priority) {
            where->second.second=priority;
            if (priority>where->second.first) {
                ///FIXME want to rebuild priority
                //std::cout<<"Rebuilding Message Map"<<'\n';
                mNextMessage.updatePriority(where->first,1,priority,true);
            }
        }
    }else {
        mActivePriorities[id]=std::pair<double,double>(priority,priority);
        mNextMessage.push(id,1,priority);
    }
}
void SpaceNode::pullFromRNs(size_t howMany) {
    size_t currentlyQueued=waitingMessagesChild();
    if (currentlyQueued<howMany) {
        howMany-=currentlyQueued;
    }else howMany=0;
    for (size_t i=0;i<howMany;++i) {
        std::vector<UUID> saveditems;
        size_t nearRNsSize=mNearRNs.size();
        bool retval=false;
        Message msg;
        for (size_t j=0;j<nearRNsSize&&!retval;++j) {
            UUID nearrn=mNearRNs.front();
            mNearRNs.pop();
            retval=gSpaceNodes[nearrn]->pullbyrn(id(),msg);
            if(retval) {
                saveditems.push_back(nearrn);
            }else {
                std::tr1::unordered_set<UUID,UUID::Hasher>::iterator where=mActiveRNs.find(nearrn);
                if (where!=mActiveRNs.end()) {
                    mActiveRNs.erase(where);
                }
            }
            if (retval) {
                SpaceNode *child=gSpaceNodes[oSeg[msg.dest]->spaceServerNode];
                double priority=gPriority(msg.source,msg.dest);
                getQueueByUUID(mChildMessageQueue,child->id()).push(msg,msg.size,priority);//FIXME really prioritize that way?
                child->notifyNewParentMessage(id(),priority);//FIXME
            }
        }
        for (std::vector<UUID>::iterator iter=saveditems.begin(),itere=saveditems.end();iter!=itere;++iter) {
            mNearRNs.push(*iter,1,gLocationPriority(bounds().center(),gSpaceNodes[*iter]->bounds().center()));
        }
    }

}

void SpaceNode::notifyNewParentMessage(const UUID& id, double priority) {
    if (!mRNPrioritySet) {
        double total=0;
        for (SpaceNodeMap::iterator i=gSpaceNodes.begin(),ie=gSpaceNodes.end();i!=ie;++i) {
            if (i->first!=this->id()&&mKnownSpaceNodes->find(i->first)==mKnownSpaceNodes->end()) {
                if (i->second->mParent==NULL) {//only update space nodes who don't have parents
                    total+=gLocationPriority(bounds().center(),i->second->bounds().center());
                }
            }
        }
        if (total) {
            mDefaultParentPriority=total; 
        }
        mRNPrioritySet=true;
    }
    mNearSpaces.updatePriority(id,1,mDefaultParentPriority,true);
}

bool SpaceNode::pullbychild(const UUID &childid, Message&msg) {
    FairQueue<Message>*q=&getQueueByUUID(mChildMessageQueue,childid);    
    if (q->empty()) {
        return false;
    }else {
        msg=q->front();
        q->pop();
        return true;
    }
    
}

void SpaceNode::pullFromSpaces(size_t howMany) {


    size_t currentlyQueued=waitingMessagesOH();
    if (currentlyQueued<howMany) {
        howMany-=currentlyQueued;
    }else howMany=0;
    for (size_t i=0;i<howMany;++i) {
        std::vector<UUID> saveditems;
        size_t nearSpacesSize=mNearSpaces.size();
        bool retval=false;
        Message msg;
        for (size_t j=0;j<nearSpacesSize&&!retval;++j) {
            UUID nearspace=mNearSpaces.front();
            mNearSpaces.pop();
            if(mParent) {
                if (nearspace==mParent->id()) {
                    retval=gSpaceNodes[nearspace]->pullbychild(id(),msg);
                }else {
                    retval=gSpaceNodes[nearspace]->pull(id(),msg);
                }
            }else {
                retval=gSpaceNodes[nearspace]->pullbyparent(msg);
            }
            if(retval) {
                saveditems.push_back(nearspace);
            }else {
                std::tr1::unordered_set<UUID,UUID::Hasher>::iterator where=mActiveSpaces.find(nearspace);
                if (where!=mActiveSpaces.end()) {
                    mActiveSpaces.erase(where);
                }
            }
            if (retval) {
                ObjectData * od=oSeg[msg.dest];
                if (od->spaceServerNode!=id()) {
                    //guess I'm a superserver that pulled from my children and now needs to stash it away in some queue
                    assert(!mParent);
                    SpaceNode* rn=gSpaceNodes[od->spaceServerNode]->mParent;
                    getQueueByUUID(mRNMessageQueue,rn->id()).push(msg,msg.size,gPriority(msg.source,msg.dest));
                    rn->notifyNewRNMessage(id());
                }else {
                    assert(mParent);
                    ObjectHost *oh=gObjectHosts[od->objectHost];
                    getQueueByUUID(mOHMessageQueue,oh->queuePerObject()?msg.dest:oh->id()).push(msg,msg.size,gPriority(msg.source,msg.dest));
                    oh->notifyNewSpaceMessage(oh->queuePerObject()?msg.dest:id());
                    //FIXME should be done on SpaceNodes pullFromOH(1);
                }
            }
        }
        for (std::vector<UUID>::iterator iter=saveditems.begin(),itere=saveditems.end();iter!=itere;++iter) {
            if (mParent&&*iter==mParent->id()) {
                mNearSpaces.push(*iter,1,mDefaultParentPriority);//FIXME parent priority adjustment
            }else{
                mNearSpaces.push(*iter,1,gLocationPriority(bounds().center(),gSpaceNodes[*iter]->bounds().center()));
            }
        }
    }
}
void SpaceNode::notifyNewSpaceMessage(const UUID&spaceId) {
    std::tr1::unordered_set<UUID,UUID::Hasher>::iterator where=mActiveSpaces.find(spaceId);
    if (where==mActiveSpaces.end()) {
        mNearSpaces.push(spaceId,1,gLocationPriority(bounds().center(),gSpaceNodes[spaceId]->bounds().center()));
        mActiveSpaces.insert(spaceId);
    }
}

void SpaceNode::notifyNewRNMessage(const UUID&spaceId) {
    std::tr1::unordered_set<UUID,UUID::Hasher>::iterator where=mActiveRNs.find(spaceId);
    if (where==mActiveRNs.end()) {
        mNearRNs.push(spaceId,1,gLocationPriority(bounds().center(),gSpaceNodes[spaceId]->bounds().center()));
        mActiveRNs.insert(spaceId);
    }
}


void SpaceNode::pullFromOH(size_t howMany) {
    size_t currentlyQueued=waitingMessagesSpace();    
    if (currentlyQueued<howMany) {
        howMany-=currentlyQueued;
    }else howMany=0;
    for (size_t i=0;i<howMany;++i) {
        Message msg;
        bool ret=pullfromoh(msg);
        if(ret) {
            UUID spaceNodeId=oSeg[msg.dest]->spaceServerNode;//FIXME OSEG lookup
            double priority=gPriority(msg.source,msg.dest);
            if (mKnownSpaceNodes->find(spaceNodeId)==mKnownSpaceNodes->end()){
                if (mParent) {
                    static int countera=0;
                    ++countera;
                    mParentMessageQueue.push(msg,msg.size,priority);
                    mParent->notifyNewSpaceMessage(id());
                }else {
                    assert(0);//should only be called on child space nodes
                }
            }else {
                    static int countera=0;
                    ++countera;

                gSpaceNodes[spaceNodeId]->notifyNewSpaceMessage(id());
                getQueueByUUID(mSpaceMessageQueue,spaceNodeId).push(msg,msg.size,priority);
            }
        }
    }
}
size_t SpaceNode::waitingMessagesOH(){
    size_t retval=0;
    for (KeyMessageQueue::iterator i=mOHMessageQueue.begin(),
             ie=mOHMessageQueue.end();
         i!=ie;
         ++i) {
        retval+=i->second.size();
    }
    return retval;
}
size_t SpaceNode::waitingMessagesSpace(){
    size_t retval=0;
    for (KeyMessageQueue::iterator i=mSpaceMessageQueue.begin(),
             ie=mSpaceMessageQueue.end();
         i!=ie;
         ++i) {
        retval+=i->second.size();
    }
    return retval;
}

size_t SpaceNode::waitingMessagesChild(){
    size_t retval=0;
    for (KeyMessageQueue::iterator i=mChildMessageQueue.begin(),
             ie=mChildMessageQueue.end();
         i!=ie;
         ++i) {
        retval+=i->second.size();
    }
    return retval;
}

size_t SpaceNode::waitingMessagesRN(){
    size_t retval=0;
    for (KeyMessageQueue::iterator i=mRNMessageQueue.begin(),
             ie=mRNMessageQueue.end();
         i!=ie;
         ++i) {
        retval+=i->second.size();
    }
    return retval;
}


size_t SpaceNode::waitingMessagesParent(){
    return mParentMessageQueue.size();
}

bool SpaceNode::pull(const UUID&space, Message&msg){
    FairQueue<Message>*q=&getQueueByUUID(mSpaceMessageQueue,space);
    if (q->empty()) {
        return false;
    }else {
        msg=q->front();
        q->pop();
        return true;
    }
}

bool SpaceNode::pullbyoh(const UUID&oh, Message&msg){
    FairQueue<Message>*q=&getQueueByUUID(mOHMessageQueue,oh);
    if (q->empty()) {
        return false;
    }else {
        msg=q->front();
        q->pop();
        return true;
    }
}

bool SpaceNode::pullbyparent(Message&msg) {
    if (mParentMessageQueue.empty())
        return false;
    static int counter=0;
    ++counter;
    msg=mParentMessageQueue.front();
    mParentMessageQueue.pop();
    return true;
}

bool SpaceNode::pullbyrn(const UUID&rn, Message&msg) {
    FairQueue<Message>*q=&getQueueByUUID(mRNMessageQueue,rn);
    if (q->empty()) {
        return false;
    }else {
        msg=q->front();
        q->pop();
        return true;
    }
}


bool SpaceNode::pullfromoh(Message&msg) {
    std::vector<UUID> saveditems;
    size_t nextMessageSize=mNextMessage.size();
    bool retval=false;
    for (size_t i=0;i<nextMessageSize&&!retval;++i){
        UUID nextitem=mNextMessage.front();
        saveditems.push_back(nextitem);
        mNextMessage.pop();
        std::tr1::unordered_map<UUID,ObjectHost*,UUID::Hasher>::iterator where=gObjectHosts.find(nextitem);
        ObjectHost *oh=NULL;
        if (where==gObjectHosts.end()) {
            oh=gObjectHosts[oSeg[nextitem]->objectHost];
        }else{
            oh=where->second;
        }
        retval= oh->pullbyspace(oh->queuePerObject()?nextitem:id(),msg);
        if (retval) {
            double priority;
            if (oh->getPriority(oh->queuePerObject()?nextitem:id(),priority)) {
                mNextMessage.push(nextitem,1,priority);
            }
        }
    }
    /*
    for (std::vector<UUID>::iteartor i=saveditems.begin(),ie=saveditems.end();i!=ie;++i) {
        
    }
    */
    return retval;
}

} }

