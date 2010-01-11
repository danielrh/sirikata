#include <util/Standard.hh>
#include <util/UUID.hpp>
#include "ObjectHost.hpp"
#include "OSeg.hpp"
#include "Priority.hpp"
#include "SpaceNode.hpp"
#include "Generator.hpp"
#include "Random.hpp"
namespace Sirikata { namespace QueueBench {
std::tr1::unordered_map<UUID,ObjectHost*,UUID::Hasher>gObjectHosts;
ObjectHost::ObjectHost(bool streamPerObject,const ObjectKnowledgeDescription &knowledge, bool objectMessageQueueIsFair):mName(pseudorandomUUID()),mKnowledge(knowledge),mPullOrder(true) {
    mLastHopKnowledge=mKnowledge;
    mObjectMessageQueueIsFair=objectMessageQueueIsFair;
    gObjectHosts[mName]=this;       
    mStreamPerObject=streamPerObject;
    mCurrentPriority=0;
    mRestoredPullOrder=0;
}
void ObjectHost::addObject(const UUID&obj){
    mObjects.insert(obj);
    UUID spaceNode=oSeg[obj]->spaceServerNode;
    if (mSpaceNodePriority.find(spaceNode)==mSpaceNodePriority.end()) {
        mSpaceNodePriority[spaceNode]=gKnowledgePriority(oSeg.mUUIDs.size()?oSeg.mUUIDs[0]:obj,obj,mLastHopKnowledge);
    }else {            
        mSpaceNodePriority[spaceNode]+=gKnowledgePriority(oSeg.mUUIDs.size()?oSeg.mUUIDs[0]:obj,obj,mLastHopKnowledge);
    }
}
void ObjectHost::restorePullOrder() {
    if (mStreamPerObject) {
        for (ObjectSet::iterator i=mObjects.begin(),ie=mObjects.end();i!=ie;++i) {
            mPullOrder.push(*i,1,gKnowledgePriority(oSeg.mUUIDs[0],*i,mLastHopKnowledge));
        }
    }else {
        for (SpaceNodePriorityMap::iterator i=mSpaceNodePriority.begin(),ie=mSpaceNodePriority.end();i!=ie;++i) {
            mPullOrder.push(i->first,1,i->second);
        }
    }
    mDefunctObjects.clear();
}

void ObjectHost::notifyNewSpaceMessage(const UUID&spaceOrObj){
    assert(mRestoredPullOrder==mDefunctObjects.size()+mPullOrder.size());
    checkBookkeeping();
    ObjectSet::iterator where=mDefunctObjects.find(spaceOrObj);
    if (where!=mDefunctObjects.end()) {
        mDefunctObjects.erase(where);        
        if (mStreamPerObject) {
            mPullOrder.push(spaceOrObj,1,gKnowledgePriority(oSeg.mUUIDs[0],spaceOrObj,mLastHopKnowledge));        
        }else {
            SpaceNodePriorityMap::iterator where=mSpaceNodePriority.find(spaceOrObj);
            assert(where!=mSpaceNodePriority.end());
            mPullOrder.push(spaceOrObj,1,where->second);
        }
        assert(mDefunctObjects.find(spaceOrObj)==mDefunctObjects.end());
    }
    checkBookkeeping();
    assert(mRestoredPullOrder==mDefunctObjects.size()+mPullOrder.size());
}
bool ObjectHost::pull(const UUID &uuid, Message&msg){
    if(!mRestoredPullOrder) {
        restorePullOrder();
        mRestoredPullOrder=mDefunctObjects.size()+mPullOrder.size();
    }
    if (mStreamPerObject) {
        ObjectData *od=oSeg[uuid];
        if (od) {
            return gSpaceNodes[od->spaceServerNode]->pullbyoh(uuid,msg);
        }
        return false;
    }else {
        ObjectData *od=oSeg[uuid];
        if (od) {
            return gSpaceNodes[od->spaceServerNode]->pullbyoh(od->objectHost,msg);
        }else if (gSpaceNodes.find(uuid)!=gSpaceNodes.end()) {
            return gSpaceNodes[uuid]->pullbyoh(od->objectHost,msg);
        }else {
            NOT_IMPLEMENTED();
            return false;
        }
    }
}

void ObjectHost::checkBookkeeping() {
    if (!mRestoredPullOrder)
        return;
    return;
        ObjectSet test=mObjects;
        ObjectSet defunctObjects=mDefunctObjects;
        FairQueue<UUID> pullOrder=mPullOrder;
        std::vector<std::pair<UUID,double> > pulled;
        while (!pullOrder.empty()) {
            UUID frnt=pullOrder.front();
            assert(test.find(frnt)!=test.end());
            pulled.push_back(std::pair<UUID,double>(frnt,pullOrder.frontPriority()));
            test.erase(frnt);
            pullOrder.pop();
        }
        while (!test.empty()){
            UUID frnt=*test.begin();
            assert(defunctObjects.find(frnt)!=defunctObjects.end());
            defunctObjects.erase(frnt);
            test.erase(test.begin());
        }
        assert(test.empty()&&defunctObjects.empty());

}
bool ObjectHost::pull(Message&msg){
    if(!mRestoredPullOrder) {
        restorePullOrder();
        mRestoredPullOrder=mDefunctObjects.size()+mPullOrder.size();
    }
    bool retval=false;
    std::vector<UUID> saved;
    size_t i=0;
    assert(mRestoredPullOrder==mDefunctObjects.size()+mPullOrder.size());
    checkBookkeeping();
    SpaceNode*ss=NULL;
    if (mStreamPerObject) {

        size_t pullOrderSize=mPullOrder.size();
        for(i=0;i<pullOrderSize&&!retval;++i) {   
            UUID objid= mPullOrder.front();
            mPullOrder.pop();
            UUID spaceid=oSeg[objid]->spaceServerNode;
            ss=gSpaceNodes[spaceid];
            retval=ss->pullbyoh(objid,msg);
            if (retval||!gNoLatePush) {
                saved.push_back(objid);
            }else {
                ss=NULL;
                bool firstPost=mDefunctObjects.insert(objid).second;
                assert(firstPost);
                assert(mRestoredPullOrder==mDefunctObjects.size()+mPullOrder.size()&&(saved.empty()||!saved.empty()));
                checkBookkeeping();    
            }
        }
        for (std::vector<UUID>::iterator iter=saved.begin(),itere=saved.end();iter!=itere;++iter) {           
            assert(mDefunctObjects.find(*iter)==mDefunctObjects.end());
            mPullOrder.push(*iter,1,gKnowledgePriority(oSeg.mUUIDs[0],*iter,mLastHopKnowledge));
            checkBookkeeping();    
        }      
    }else{
        size_t pullOrderSize=mPullOrder.size();
        for(i=0;i<pullOrderSize&&!retval;++i) {   
            UUID spaceid= mPullOrder.front();
            ss=gSpaceNodes[spaceid];
            retval=ss->pullbyoh(id(),msg);
            if (retval||!gNoLatePush) {
                saved.push_back(spaceid);
            }else {
                ss=NULL;
                mDefunctObjects.insert(spaceid);
            }
            mPullOrder.pop();
        }
        for (std::vector<UUID>::iterator iter=saved.begin(),itere=saved.end();iter!=itere;++iter) {
            mPullOrder.push(*iter,1,mSpaceNodePriority[*iter]);
        }
    }
    checkBookkeeping();    
    if(ss) {
        ss->pullFromSpaces(1);
    }
    checkBookkeeping();    
    assert(mRestoredPullOrder==mDefunctObjects.size()+mPullOrder.size()&&(saved.empty()||!saved.empty()));
    return retval;    
}
double ObjectHost::ohMessagePriority(const Message&msg){
    return gKnowledgePriority(msg.source,msg.dest,mKnowledge);
}
void ObjectHost::insertMessage (const Message&msg){
    UUID sserver=oSeg[msg.source]->spaceServerNode;
    UUID name=mStreamPerObject?msg.source:sserver;
    FairQueue<Message>*q=getObjectMessageOrder(name);
    q->push(msg,msg.size,ohMessagePriority(msg));
    gSpaceNodes[sserver]->notifyNewOHMessage(mStreamPerObject?msg.source:oSeg[msg.source]->objectHost,ohMessagePriority(q->front()));
}

bool ObjectHost::getPriority(const UUID&uuid,double&priority) {
    FairQueue<Message>*q=getObjectMessageOrder(uuid);
    if (q->empty())
        return false;
    priority=ohMessagePriority(q->front());
    return true;

}
FairQueue<Message>*ObjectHost::getObjectMessageOrder(const UUID&name){
    std::tr1::unordered_map<UUID,FairQueue<Message>,UUID::Hasher >::iterator where=mObjectMessageOrder.find(name);
    if (where==mObjectMessageOrder.end()) {
        mObjectMessageOrder.insert(std::tr1::unordered_map<UUID,FairQueue<Message>,UUID::Hasher >::value_type(name,FairQueue<Message>(mObjectMessageQueueIsFair)));
        where=mObjectMessageOrder.find(name);
    }
    return &where->second;
}

bool ObjectHost::pullbyspace(const UUID&uuid,Message&msg){
    FairQueue<Message>*q=getObjectMessageOrder(uuid);
    if (q->empty())
        return false;
    static int counter=0;
    msg=q->front();
    q->pop();
    return true;

}
size_t ObjectHost::objectMessageQueueSize()const{
    size_t retval=0;
    for (std::tr1::unordered_map<UUID,FairQueue<Message>,UUID::Hasher >::const_iterator i=mObjectMessageOrder.begin(),ie=mObjectMessageOrder.end();i!=ie;++i) {
        retval+=i->second.size();
    }
    return retval;
}
} }
