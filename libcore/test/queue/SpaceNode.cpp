#include "util/Standard.hh"
#include "util/UUID.hpp"
#include "SpaceNode.hpp"
#include "ObjectHost.hpp"
#include "OSeg.hpp"
#include "Priority.hpp"
#include "Random.hpp"
namespace Sirikata{ namespace QueueBench {
std::tr1::unordered_map<UUID,SpaceNode*,UUID::Hasher>gSpaceNodes;
SpaceNode::SpaceNode(const BoundingBox3d3f & box):mBounds(box),mName(pseudorandomUUID()){
    gSpaceNodes[id()]=this;
    mKnownSpaceNodes=&gSpaceNodes;
    mRNPrioritySet=false;
}

void SpaceNode::setKnownSpaceNodes(const SpaceNodeMap*snm) {
    mKnownSpaceNodes=snm;
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
    return new SpaceNode(newbounds);
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

void SpaceNode::notifyNewOHMessage(const UUID&id, double priority) {
    PriorityMap::iterator where=mActivePriorities.find(id);
    if (where!=mActivePriorities.end()) {
        if (where->second.second<priority) {
            where->second.second=priority;
            if (priority>where->second.first*2) {
                ///FIXME want to rebuild priority
                //std::cout<<"Rebuilding Message Map"<<'\n';
                mNextMessage.updatePriority(where->first,1,priority);
            }
        }
    }else {
        mActivePriorities[id]=std::pair<double,double>(priority,priority);
        mNextMessage.push(id,1,priority);
    }
}
void SpaceNode::pullFromSpaces(size_t howMany) {
    if (!mRNPrioritySet) {
        double total=0;
        for (SpaceNodeMap::iterator i=gSpaceNodes.begin(),ie=gSpaceNodes.end();i!=ie;++i) {
            if (i->first!=id()&&mKnownSpaceNodes->find(i->first)==mKnownSpaceNodes->end()) {
                total+=gLocationPriority(bounds().center(),i->second->bounds().center());
            }
        }
        if (total) {
            mNearSpaces.push(UUID::null(),1,total);            
        }
        mRNPrioritySet=true;
    }
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
            retval=gSpaceNodes[nearspace]->pull(id(),msg);
            if(retval||mKnownSpaceNodes->find(nearspace)==mKnownSpaceNodes->end()) {
                saveditems.push_back(nearspace);
            }else {
                std::tr1::unordered_set<UUID,UUID::Hasher>::iterator where=mActiveSpaces.find(nearspace);
                if (where!=mActiveSpaces.end()) {
                    mActiveSpaces.erase(where);
                }
            }
            if (retval) {
                ObjectHost *oh=gObjectHosts[oSeg[msg.dest]->objectHost];
                mOHMessageQueue[oh->queuePerObject()?msg.dest:oh->id()].push(msg,msg.size,gPriority(msg.source,msg.dest));
                oh->notifyNewSpaceMessage(oh->queuePerObject()?msg.dest:id());
                pullFromOH(1);
            }
        }
        for (std::vector<UUID>::iterator iter=saveditems.begin(),itere=saveditems.end();iter!=itere;++iter) {
            mNearSpaces.push(*iter,1,gLocationPriority(bounds().center(),gSpaceNodes[*iter]->bounds().center()));
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
                mRNMessageQueue.push(msg,msg.size,priority);
            }else {
                gSpaceNodes[spaceNodeId]->notifyNewSpaceMessage(id());
                mSpaceMessageQueue[spaceNodeId].push(msg,msg.size,priority);
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
size_t SpaceNode::waitingMessagesRN(){
    return mRNMessageQueue.size();
}

bool SpaceNode::pull(const UUID&space, Message&msg){
    FairQueue<Message>*q=&mSpaceMessageQueue[space];
    if (q->empty()) {
        return false;
    }else {
        msg=q->front();
        q->pop();
        return true;
    }
}

bool SpaceNode::pullbyoh(const UUID&oh, Message&msg){
    FairQueue<Message>*q=&mOHMessageQueue[oh];
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

