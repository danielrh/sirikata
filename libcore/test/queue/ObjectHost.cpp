#include <util/Standard.hh>
#include <util/UUID.hpp>
#include "ObjectHost.hpp"
#include "OSeg.hpp"
#include "Priority.hpp"
#include "SpaceNode.hpp"
#include "Generator.hpp"
namespace Sirikata { namespace QueueBench {
std::tr1::unordered_map<UUID,ObjectHost*,UUID::Hasher>gObjectHosts;
ObjectHost::ObjectHost(bool streamPerObject,bool distanceKnowledge,bool remoteRadiusKnowledge,bool localRadiusKnowledge):mName(UUID::random()) {
    gObjectHosts[mName]=this;       
    mStreamPerObject=streamPerObject;
    mLocalObjectRadiusKnowledge=localRadiusKnowledge;
    mRemoteObjectRadiusKnowledge=remoteRadiusKnowledge;
    mDistanceKnowledge=distanceKnowledge;
    mCurrentPriority=0;
    mRestoredPullOrder=0;
}
void ObjectHost::addObject(const UUID&obj){
    mObjects.insert(obj);
    UUID spaceNode=oSeg[obj]->spaceServerNode;
    if (mSpaceNodePriority.find(spaceNode)==mSpaceNodePriority.end()) {
        mSpaceNodePriority[spaceNode]=gPriority(obj,obj);
    }else {            
        mSpaceNodePriority[spaceNode]+=gPriority(obj,obj);
    }
}
void ObjectHost::restorePullOrder() {
    if (mStreamPerObject) {
        for (ObjectSet::iterator i=mObjects.begin(),ie=mObjects.end();i!=ie;++i) {
            mPullOrder.push(*i,1,gPriority(*i,*i));
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
            mPullOrder.push(spaceOrObj,1,gPriority(spaceOrObj,spaceOrObj));        
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

        FairQueue<UUID> preBookkeeping;        
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
            mPullOrder.push(*iter,1,gPriority(*iter,*iter));
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
    double priority=1;
    if (mDistanceKnowledge) {
        priority=gPriority(msg.source,msg.dest);
        if (!mRemoteObjectRadiusKnowledge) {
            priority/=oSeg[msg.dest]->radialSize;
        }
        if(!mLocalObjectRadiusKnowledge) {
            priority/=oSeg[msg.source]->radialSize;
        }
    }else {
        if(mLocalObjectRadiusKnowledge) {
            priority*=oSeg[msg.source]->radialSize;
        }
        if (mRemoteObjectRadiusKnowledge) {
            priority*=oSeg[msg.dest]->radialSize;
        }
    }
    return priority;
}
void ObjectHost::insertMessage (const Message&msg){
    UUID sserver=oSeg[msg.source]->spaceServerNode;
    UUID name=mStreamPerObject?msg.source:sserver;
    FairQueue<Message>*q=&mObjectMessageOrder[name];
    q->push(msg,msg.size,ohMessagePriority(msg));
    gSpaceNodes[sserver]->notifyNewOHMessage(mStreamPerObject?msg.source:oSeg[msg.source]->objectHost,ohMessagePriority(q->front()));
}

bool ObjectHost::getPriority(const UUID&uuid,double&priority) {
    FairQueue<Message>*q=&mObjectMessageOrder[uuid];
    if (q->empty())
        return false;
    priority=ohMessagePriority(q->front());
    return true;

}
bool ObjectHost::pullbyspace(const UUID&uuid,Message&msg){
    FairQueue<Message>*q=&mObjectMessageOrder[uuid];
    if (q->empty())
        return false;
    msg=q->front();
    q->pop();
    return true;

}

} }
