#include "proximity/Platform.hpp"
#include "util/ObjectReference.hpp"
#include "Prox_Sirikata.pbj.hpp"
#include "proximity/ProximitySystem.hpp"
#include "prox/QueryHandler.hpp"
#include "prox/QueryEventListener.hpp"
#include "prox/QueryChangeListener.hpp"
#include "ProxBridge.hpp"
#include "network/TCPStreamListener.hpp"
#include "network/TCPStream.hpp"
#include "options/Options.hpp"
#include "network/IOServiceFactory.hpp"
//#include "Sirikata.pbj.hpp"
namespace Sirikata { namespace Proximity {

void ProxBridge::newObjectStreamCallback(Network::Stream*newStream, Network::Stream::SetCallbacks&setCallbacks) {
    std::tr1::shared_ptr<std::vector<ObjectReference> > ref(new std::vector<ObjectReference>());
    std::tr1::shared_ptr<Network::Stream> stream(newStream);
    setCallbacks(
        std::tr1::bind(&ProxBridge::disconnectionCallback,this,stream,ref,_1,_2),
        std::tr1::bind(&ProxBridge::incomingMessage,this,stream,ref,_1));
}

void ProxBridge::update(const Duration&duration,const std::tr1::weak_ptr<Prox::QueryHandler>&listen) {
    std::tr1::shared_ptr<Prox::QueryHandler> listener=listen.lock();
    if (listener) {
        listener->tick(Prox::Time((Time::now()-Time::epoch()).toMicroseconds()));
        Network::IOServiceFactory::dispatchServiceMessage(mIO,duration,std::tr1::bind(&ProxBridge::update,this,duration,listen));
    }
}

ProxBridge::ProxBridge(Network::IOService&io,const String&options, Prox::QueryHandler*handler, const Callback&cb):mIO(&io),mListener(new Network::TCPStreamListener(io)),mQueryHandler(handler),mCallback(cb) {
    OptionValue*port;    
    OptionValue*updateDuration;
    InitializeClassOptions("proxbridge",this,
                          port=new OptionValue("port","6408",OptionValueType<String>(),"sets the port that the proximity bridge should listen on"),
                          updateDuration=new OptionValue("updateDuration","60ms",OptionValueType<Duration>(),"sets the ammt of time between proximity updates"),
						  NULL);
    (mOptions=OptionSet::getOptions("proxbridge",this))->parse(options);
    std::tr1::weak_ptr<Prox::QueryHandler> phandler=mQueryHandler;
    Network::IOServiceFactory::dispatchServiceMessage(&io,updateDuration->as<Duration>(),std::tr1::bind(&ProxBridge::update,this,updateDuration->as<Duration>(),phandler));
    mListener->listen(Network::Address("127.0.0.1",port->as<String>()),
                      std::tr1::bind(&ProxBridge::newObjectStreamCallback,this,_1,_2));
   
}
ProxBridge::~ProxBridge() {
    std::tr1::weak_ptr<Prox::QueryHandler> listener=mQueryHandler;
    mQueryHandler=std::tr1::shared_ptr<Prox::QueryHandler>();
    while (listener.lock()) {
#ifdef _WIN32
        Sleep(0);
#else
        sleep(0);
#endif
    }
    while (!mObjectStreams.empty()) {
        delObj(mObjectStreams.begin());
    }
    delete mListener;
}

ProximitySystem::OpaqueMessageReturnValue ProxBridge::processOpaqueProximityMessage(std::vector<ObjectReference>&newObjectReferences,
                                               const ObjectReference*object,    
                                               const Sirikata::Protocol::IMessage&msg,
                                               const void *optionalSerializedMessage,
                                               size_t optionalSerializedMessageSize) {
    ObjectStateMap::iterator where=mObjectStreams.end();
    if (object) {
        where=mObjectStreams.find(*object);
    }
    return processOpaqueProximityMessage(newObjectReferences,where,msg,optionalSerializedMessage,optionalSerializedMessageSize);
}
ProximitySystem::OpaqueMessageReturnValue ProxBridge::processOpaqueProximityMessage(std::vector<ObjectReference>&newObjectReferences,
                                               ProxBridge::ObjectStateMap::iterator where,
                                               const Sirikata::Protocol::IMessage&msg,
                                               const void *optionalSerializedMessage,
                                               size_t optionalSerializedMessageSize) {
    int numMessages=msg.message_names_size();
    if (numMessages!=msg.message_arguments_size()) {
        return OBJECT_NOT_DESTROYED; //FIXME should we assume the rest equal the first
    }
    OpaqueMessageReturnValue retval=OBJECT_NOT_DESTROYED;
    for (int i=0;i<numMessages;++i) {
        if (msg.message_names(i)=="RetObj") {
            Sirikata::Protocol::RetObj new_obj;
            if (new_obj.ParseFromString(msg.message_arguments(i))) {
                ObjectReference ref;
                where=this->newObj(ref,new_obj);
                newObjectReferences.push_back(ref);
            }
        }else if (where!=mObjectStreams.end()) {
            if (msg.message_names(i)=="NewProxQuery") {
                Sirikata::Protocol::NewProxQuery new_query;
                if (new_query.ParseFromString(msg.message_arguments(i))) {
                    this->newProxQuery(where,new_query,msg.message_arguments(i).data(),msg.message_arguments(i).size());
                }
            }
            
            if (msg.message_names(i)=="DelProxQuery") {
                Sirikata::Protocol::DelProxQuery del_query;
                if (del_query.ParseFromString(msg.message_arguments(i))) {
                    this->delProxQuery(where,del_query,msg.message_arguments(i).data(),msg.message_arguments(i).size());
                }
            }
            if (msg.message_names(i)=="ObjLoc"){
                Sirikata::Protocol::ObjLoc obj_loc;
                if (obj_loc.ParseFromString(msg.message_arguments(i))) {
                    this->objLoc(where,obj_loc,msg.message_arguments(i).data(),msg.message_arguments(i).size());
                }
                
            }
            if (msg.message_names(i)=="DelObj") {
                Sirikata::Protocol::DelObj del_obj;
                if (del_obj.ParseFromString(msg.message_arguments(i))) {
                    this->delObj(where);
                    retval=OBJECT_DELETED;
                    return retval;//once an object is deleted, the iterator is invalid
                }
            }
        }
    }    
    return retval;
}
ObjectReference ProxBridge::newObj(const Sirikata::Protocol::IRetObj&obj_status,
                        const void *optionalSerializedReturnObjectConnection,
                        size_t optionalSerializedReturnObjectConnectionSize){
    ObjectReference retval;
    this->newObj(retval,obj_status);
    return retval;
}
ProxBridge::ObjectStateMap::iterator ProxBridge::newObj(ObjectReference&retval,
                                                        const Sirikata::Protocol::IRetObj&obj_status) {
    
    const Sirikata::Protocol::IObjLoc location=obj_status.location();
    
    BoundingSphere3f sphere=obj_status.bounding_sphere();
    Prox::BoundingSphere3f boundingSphere(sphere.center().convert<Prox::Vector3<Prox::BoundingSphere3f::real> >(),
                                          sphere.radius());
    ObjectStateMap::iterator where;
    UUID object_reference(obj_status.object_reference());
    retval=ObjectReference(object_reference);
    if ((where=mObjectStreams.find(ObjectReference(object_reference)))!=mObjectStreams.end()) { 
        where->second->mObject->bounds(boundingSphere);
        objLoc(where,location);
    } else {
        Prox::Object::PositionVectorType position(Prox::Time((location.timestamp()-Time::epoch()).toMicroseconds()),
                                                  location.position().convert<Prox::Object::PositionVectorType::CoordType>(),
                                                  location.velocity().convert<Prox::Vector3f>());
        Prox::ObjectID id(object_reference.getArray().begin(),UUID::static_size);
        Prox::Object * obj=new Prox::Object(id,
                                            position,
                                            boundingSphere);
        ObjectState*state;
        mObjectStreams[retval]=state=new ObjectState(NULL);
        assert(state->mObject==NULL);
        state->mObject=obj;
        state->mQueries.clear();
        mQueryHandler->registerObject(obj);
        where=mObjectStreams.find(retval);
    }
    return where;
}

namespace {
UUID convertProxObjectId(const Prox::ObjectID &id) {
    return UUID((unsigned char*)id.begin(),Prox::ObjectID::static_size);
}


}
void ProxBridge::sendProxCallback(Network::Stream*stream,
                                         const ObjectReference&destination,
                                         const Sirikata::Protocol::IMessage&unaddressed_prox_callback_msg){
    std::string str;
    unaddressed_prox_callback_msg.SerializeToString(&str);
    stream->send(str.data(),str.size(),Network::ReliableOrdered);
}
class QueryListener:public Prox::QueryEventListener, public Prox::QueryChangeListener {
    ProxBridge::ObjectState* mState;
    unsigned int mID;
    ProxBridge::Callback mCallback;
public:
    QueryListener(unsigned int id, ProxBridge::ObjectState*state, const ProxBridge::Callback &cb):mState(state), mCallback(cb) {
        mID=id;
    }
    virtual ~QueryListener(){}
    virtual void queryHasEvents(Prox::Query*query){
        ObjectReference reference(convertProxObjectId(mState->mObject->id()));
        Protocol::ProxCall callback_message;
        Protocol::Message message_container;
        std::deque<Prox::QueryEvent> evts;
        query->popEvents(evts);
        std::deque<Prox::QueryEvent>::const_iterator i=evts.begin(),iend=evts.end();
        for (;i!=iend;++i) {
            if (i->type()==Prox::QueryEvent::Added||i->type()==Prox::QueryEvent::Removed) {
                callback_message.set_proximity_event(i->type()==Prox::QueryEvent::Added?Protocol::ProxCall::ENTERED_PROXIMITY:Protocol::ProxCall::EXITED_PROXIMITY);
                callback_message.set_proximate_object(convertProxObjectId(i->id()));
                callback_message.set_query_id(mID);
                message_container.add_message_arguments(std::string());
                callback_message.SerializeToString(&message_container.message_arguments(message_container.message_arguments_size()-1));
                message_container.add_message_names("ProxCall");
            }
        }
        if (message_container.message_names_size()) {
            mCallback(mState->mStream?&*mState->mStream:NULL,reference,message_container);
        }
    }
    virtual void queryPositionUpdated(Prox::Query* query, const Prox::Query::PositionVectorType& old_pos, const Prox::MotionVector3f& new_pos){}
    virtual void queryDeleted(const Prox::Query* query){delete this;}
};
void ProxBridge::newProxQuery(ObjectStateMap::iterator source,
                              const Sirikata::Protocol::INewProxQuery&new_query,
                              const void *optionalSerializedProximityQuery,
                              size_t optionalSerializedProximitySize){
    QueryMap::iterator where=source->second->mQueries.find(new_query.query_id());
    if (where!=source->second->mQueries.end()) {
        delete where->second.mQuery;
        source->second->mQueries.erase(where);
    }
    Prox::Query * query=NULL;
    if (new_query.has_min_solid_angle()||new_query.has_max_radius()) {
        QueryState*queryState=&source->second->mQueries[new_query.query_id()];
        Prox::Query::PositionVectorType pos(source->second->mObject->position());
        queryState->mOffset=Vector3d(0,0,0);
        queryState->mQueryType=new_query.stateless()?QueryState::RELATIVE_STATELESS:QueryState::RELATIVE_STATEFUL;
        if (new_query.has_absolute_center()) {
            pos.update((Time::now()-Time::epoch()).toMicroseconds(),
                       new_query.absolute_center().convert<Prox::Query::PositionVectorType::CoordType>(),
                       Prox::Query::PositionVectorType::CoordType(0,0,0));
            queryState->mOffset=new_query.absolute_center();
            queryState->mQueryType=new_query.stateless()?QueryState::ABSOLUTE_STATELESS:QueryState::ABSOLUTE_STATEFUL;
        }
        if (new_query.has_relative_center()) {
            pos+=new_query.relative_center().convert<Prox::Query::PositionVectorType::CoordType>();
            queryState->mOffset.x+=new_query.relative_center().x;
            queryState->mOffset.y+=new_query.relative_center().y;
            queryState->mOffset.z+=new_query.relative_center().z;
        }
        if (new_query.has_min_solid_angle()&&new_query.has_max_radius()) {
            queryState->mQuery=query=new Prox::Query(pos,Prox::SolidAngle(new_query.min_solid_angle()),new_query.max_radius());
        }else if (new_query.has_max_radius()) {
            queryState->mQuery=query=new Prox::Query(pos,Prox::SolidAngle::Max,new_query.max_radius());
        }else if (new_query.has_min_solid_angle()) {
            queryState->mQuery=query=new Prox::Query(pos,Prox::SolidAngle(new_query.min_solid_angle()));
        }
        mQueryHandler->registerQuery(query);
        QueryListener * ql=new QueryListener(new_query.query_id(),source->second,mCallback);
        query->addChangeListener(ql);
        query->setEventListener(ql);
    }
}
void ProxBridge::delProxQuery(ObjectStateMap::iterator source,
                              const Sirikata::Protocol::IDelProxQuery&del_query,
                              const void *optionalSerializedDelProxQuery,
                              size_t optionalSerializedDelProxQuerySize) {
    QueryMap::iterator where=source->second->mQueries.find(del_query.query_id());
    if (where!=source->second->mQueries.end()) {
        delete where->second.mQuery;
        source->second->mQueries.erase(where);        
    }
}
void ProxBridge::delObj(ObjectStateMap::iterator source){
    QueryMap::iterator i=source->second->mQueries.begin(),ie=source->second->mQueries.end();
    for (;i!=ie;++i) {
        delete i->second.mQuery;
    }
    delete source->second->mObject;
    delete source->second;
    mObjectStreams.erase(source);
}
    /**
     * Register a new proximity query.
     * The callback may come from an ASIO response thread
     */
void ProxBridge::newProxQuery(const ObjectReference&source,
                              const Sirikata::Protocol::INewProxQuery&new_query,
                              const void *optionalSerializedProximityQuery,
                              size_t optionalSerializedProximitySize){
    ObjectStateMap::iterator where=mObjectStreams.find(source);
    if (where!=mObjectStreams.end())
        this->newProxQuery(where,new_query,optionalSerializedProximityQuery,optionalSerializedProximitySize);
    else SILOG(prox,warning,"Cannot create new prox query for nonexistant object "<<source);
}
    /**
     * Since certain setups have proximity responses coming from another message stream
     * Those messages should be shunted to this function and processed
     */
void ProxBridge::processProxCallback(const ObjectReference&destination,
                                     const Sirikata::Protocol::IProxCall&prox_callback,
                                     const void *optionalSerializedProxCall,
                                     size_t optionalSerializedProxCallSize){
    ObjectStateMap::iterator where=mObjectStreams.find(destination);
    if (where!=mObjectStreams.end()) {
        Sirikata::Protocol::Message msg;
        msg.add_message_names("ProxCall");
        if (optionalSerializedProxCallSize) {
            msg.add_message_arguments(optionalSerializedProxCall,optionalSerializedProxCallSize);
        }else {
            msg.add_message_arguments(std::string());
            prox_callback.SerializeToString(&msg.message_arguments(0));
        }
        mCallback(where->second->mStream?&*where->second->mStream:NULL,destination,msg);
    }else SILOG(prox,warning,"Cannot callback to "<<destination<<" unknown stream");    
}

void ProxBridge::objLoc(ObjectStateMap::iterator where,
                        const Sirikata::Protocol::IObjLoc& obj_loc,
                        const void *optionalSerializedObjLoc,
                        size_t optionalSerializedObjLocSize){
    Prox::Object::PositionVectorType position(Prox::Time((obj_loc.timestamp()-Time::epoch()).toMicroseconds()),
                                                  obj_loc.position().convert<Prox::Object::PositionVectorType::CoordType>(),
                                                  obj_loc.velocity().convert<Prox::Vector3f>());
    
    where->second->mObject->position(position);
    for (QueryMap::iterator i=where->second->mQueries.begin(),ie=where->second->mQueries.end();i!=ie;++i) {
        if (i->second.mQueryType==QueryState::RELATIVE_STATEFUL||i->second.mQueryType==QueryState::RELATIVE_STATELESS) {
            if (i->second.mOffset.x||i->second.mOffset.y||i->second.mOffset.z) {
                Prox::Query::PositionVectorType pos(position);
                i->second.mQuery->position(pos+=i->second.mOffset.convert<Prox::Query::PositionVectorType::CoordType>());
            }else {
                i->second.mQuery->position(position);
            }
        }            
    }
}
    /**
     * The proximity management system must be informed of all position updates
     * Pass an objects position updates to this function
     */
void ProxBridge::objLoc(const ObjectReference&source, const Sirikata::Protocol::IObjLoc&loc, const void *optionalSerializedObjLoc,size_t optionalSerializedObjLocSize){
   ObjectStateMap::iterator where=mObjectStreams.find(source);
   if (where!=mObjectStreams.end()) {
       this->objLoc(where,loc,optionalSerializedObjLoc,optionalSerializedObjLocSize);
   }else SILOG(prox,warning,"Cannot update object loc for nonexistant object "<<source); 
}

    /**
     * Objects may lose interest in a particular query
     * when this function returns, no more responses will be given
     */
void ProxBridge::delProxQuery(const ObjectReference&source, const Sirikata::Protocol::IDelProxQuery&del_query,  const void *optionalSerializedDelProxQuery,size_t optionalSerializedDelProxQuerySize){
   ObjectStateMap::iterator where=mObjectStreams.find(source);
   if (where!=mObjectStreams.end()) {
       this->delProxQuery(where,del_query,optionalSerializedDelProxQuery,optionalSerializedDelProxQuerySize);
   }else SILOG(prox,warning,"Cannot delete query for nonexistant object "<<source); 
}
    /**
     * Objects may be destroyed: indicate loss of interest here
     */
void ProxBridge::delObj(const ObjectReference&source, const Sirikata::Protocol::IDelObj&del_obj, const void *optionalSerializedDelObj,size_t optionalSerializedDelObjSize){
   ObjectStateMap::iterator where=mObjectStreams.find(source);
   if (where!=mObjectStreams.end()) {
       this->delObj(where);
   }else SILOG(prox,warning,"Cannot delete nonexistant object "<<source); 
}
void ProxBridge::incomingMessage(const std::tr1::weak_ptr<Network::Stream>&strm,
                                 const std::tr1::shared_ptr<std::vector<ObjectReference> >&ref,
                                 const Network::Chunk&data) {
    Protocol::Message msg;
    if (data.size()&&msg.ParseFromArray(&*data.begin(),data.size())) {
        size_t old_size=ref->size();
        if (msg.has_source_object()) {
            ObjectReference source_object(msg.source_object());
            if (processOpaqueProximityMessage(*ref,&source_object,msg,&*data.begin(),data.size())==OBJECT_DELETED) {
                std::vector<ObjectReference>::iterator where=std::find(ref->begin(),ref->end(),source_object);
                if (where!=ref->end()) {
                    ref->erase(where);
                }
            }
        }else if (old_size==1) {
            if (processOpaqueProximityMessage(*ref,&*ref->begin(),msg,&*data.begin(),data.size())==OBJECT_DELETED) {
                ref->resize(0);
            }
        }else {
            std::vector<ObjectReference> newObjects;
            if (processOpaqueProximityMessage(newObjects,NULL,msg,&*data.begin(),data.size())!=OBJECT_DELETED&&!newObjects.empty()) {
                ref->insert(ref->end(),newObjects.begin(),newObjects.end());
            }
        }
        if (old_size<ref->size()) {
            for (size_t j=old_size;j<ref->size();++j) {
                ObjectStateMap::iterator where=mObjectStreams.find((*ref)[j]);
                if (where!=mObjectStreams.end()) {
                    where->second->mStream=strm.lock();
                }
            }
        }
    }
}
void ProxBridge::disconnectionCallback(const std::tr1::shared_ptr<Network::Stream> &stream,
                                       const std::tr1::shared_ptr<std::vector<ObjectReference> >&refs,
                                       Network::Stream::ConnectionStatus status,
                                       const std::string&reason) {
    //FIXME iterate through refffs, disconnecting 'em
    if (status!=Network::Stream::Connected) {
        for (std::vector<ObjectReference>::iterator i=refs->begin(),ie=refs->end();i!=ie;++i) {
            ObjectStateMap::iterator where=mObjectStreams.find(*i);
            if (where!=mObjectStreams.end()) {
                delObj(where);
            }
        } 
        stream->close();
    }
}


} }
