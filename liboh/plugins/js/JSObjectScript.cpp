/*  Sirikata
 *  JSObjectScript.cpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <sirikata/oh/Platform.hpp>

#include "Protocol_Sirikata.pbj.hpp"

#include <sirikata/core/util/RoutableMessageHeader.hpp>
#include <sirikata/core/util/RoutableMessageBody.hpp>
#include <sirikata/core/util/KnownServices.hpp>


#include "JSObjectScript.hpp"
#include "JSObjectScriptManager.hpp"
#include "JSLogging.hpp"

#include "JSSerializer.hpp"

#include "JSEventHandler.hpp"
#include <string>
#include "JSUtil.hpp"
#include "JSObjects/JSVec3.hpp"
#include "JSObjects/JSQuaternion.hpp"

#include <sirikata/oh/ObjectHost.hpp>
#include <sirikata/core/network/IOService.hpp>

#include <sirikata/core/odp/Defs.hpp>
#include <vector>
#include <set>
#include "JSObjects/JSFields.hpp"
#include "JS_JSMessage.pbj.hpp"
#include "JSSystemNames.hpp"
#include "JSPresenceStruct.hpp"


using namespace v8;
using namespace std;
namespace Sirikata {
namespace JS {


JSObjectScript::JSObjectScript(HostedObjectPtr ho, const ObjectScriptManager::Arguments& args, JSObjectScriptManager* jMan)
 : mParent(ho),
   mManager(jMan)
{
    v8::HandleScope handle_scope;
    mContext = Context::New(NULL, mManager->mGlobalTemplate);

    mPres = NULL; //bftm change.
    
    Local<Object> global_obj = mContext->Global();
    // NOTE: See v8 bug 162 (http://code.google.com/p/v8/issues/detail?id=162)
    // The template actually generates the root objects prototype, not the root
    // itself.
    Handle<Object> global_proto = Handle<Object>::Cast(global_obj->GetPrototype());
    global_proto->SetInternalField(0, External::New(this));
    // And we add an internal field to the system object as well to make it
    // easier to find the pointer in different calls. Note that in this case we
    // don't use the prototype -- non-global objects work as we would expect.
    Local<Object> system_obj = Local<Object>::Cast(global_proto->Get(v8::String::New(JSSystemNames::ROOT_OBJECT_NAME)));
    populateSystemObject(system_obj);

    mHandlingEvent = false;


    
    const HostedObject::SpaceSet& spaces = mParent->spaces();
    if (spaces.size() > 1)
        JSLOG(fatal,"Error: Connected to more than one space.  Only enabling scripting for one space.");

    for(HostedObject::SpaceSet::const_iterator space_it = spaces.begin(); space_it != spaces.end(); space_it != spaces.end()?space_it++:space_it)
    {
        //register for scripting messages from user
        SpaceID space_id=*space_it;
        mScriptingPort = mParent->bindODPPort(space_id, Services::SCRIPTING);
        if (mScriptingPort)
            mScriptingPort->receive( std::tr1::bind(&JSObjectScript::handleScriptingMessage, this, _1, _2) );


        //register port for messaging
        mMessagingPort = mParent->bindODPPort(space_id, Services::COMMUNICATION);

        if (mMessagingPort)
            mMessagingPort->receive( std::tr1::bind(&JSObjectScript::bftm_handleCommunicationMessage, this, _1, _2) );

        space_it=spaces.find(space_id);//in case the space_set was munged in the process
    }
}


void JSObjectScript::create_entity(Vector3d& vec, String& script_name)
{

  //float WORLD_SCALE = mParent->mInputManager->mWorldScale->as<float>();

  // get the script type
  String script_type = "js";
  Sirikata::Protocol::CreateObject creator;
  Sirikata::Protocol::IConnectToSpace space = creator.add_space_properties();
  Sirikata::Protocol::IObjLoc loc = space.mutable_requested_object_loc();
  //loc.set_position(curLoc.getPosition() + Vector3d(direction(curLoc.getOrientation()))*WORLD_SCALE/3);
  loc.set_position(vec);
  //loc.set_orientation(curLoc.getOrientation());
  loc.set_velocity(Vector3f(0,0,0));
  loc.set_angular_speed(0);
  loc.set_rotational_axis(Vector3f(1,0,0));

  creator.set_mesh("http://www.sirikata.com/content/assets/cube.dae");
  creator.set_scale(Vector3f(1,1,1));
  creator.set_script(script_type);
  creator.set_script_name(script_name);
  //Sirikata::Protocol::IStringMapProperty script_args = creator.mutable_script_args();
  std::string serializedCreate;
  creator.SerializeToString(&serializedCreate);
  RoutableMessageBody body;
  body.add_message("CreateObject", serializedCreate);
  std::string serialized;
  body.SerializeToString(&serialized);

  const HostedObject::SpaceSet& spaces = mParent->spaces();
  SpaceID spaceider = *(spaces.begin());
  ODP::Endpoint dest (spaceider,mParent->getObjReference(spaceider),Services::RPC);
  mMessagingPort->send(dest, MemoryReference(serialized.data(), serialized.length()));

}


void JSObjectScript::reboot()
{
  // Need to delete the existing context? v8 garbage collects?

  v8::HandleScope handle_scope;
  mContext = Context::New(NULL, mManager->mGlobalTemplate);
  Local<Object> global_obj = mContext->Global();
  Handle<Object> global_proto = Handle<Object>::Cast(global_obj->GetPrototype());
  global_proto->SetInternalField(0, External::New(this));
  Local<Object> system_obj = Local<Object>::Cast(global_proto->Get(v8::String::New(JSSystemNames::ROOT_OBJECT_NAME)));

  populateSystemObject(system_obj);

  //delete all handlers
  for (int s=0; s < (int) mEventHandlers.size(); ++s)
      delete mEventHandlers[s];

  mEventHandlers.clear();

}

void JSObjectScript::bftm_debugPrintString(std::string cStrMsgBody) const
{
    std::cout<<"\n\n\n\nIs it working:  "<<cStrMsgBody<<"\n\n";
}


JSObjectScript::~JSObjectScript()
{
    if (mScriptingPort)
        delete mScriptingPort;

    for(JSEventHandlerList::iterator handler_it = mEventHandlers.begin(); handler_it != mEventHandlers.end(); handler_it++)
        delete *handler_it;

    mEventHandlers.clear();

    mContext.Dispose();
}



bool JSObjectScript::forwardMessagesTo(MessageService*){
    NOT_IMPLEMENTED(js);
    return false;
}

bool JSObjectScript::endForwardingMessagesTo(MessageService*){
    NOT_IMPLEMENTED(js);
    return false;
}

bool JSObjectScript::processRPC(
    const RoutableMessageHeader &receivedHeader,
    const std::string& name,
    MemoryReference args,
    MemoryBuffer &returnValue)
{
    NOT_IMPLEMENTED(js);
    return false;
}

void JSObjectScript::processMessage(
    const RoutableMessageHeader& receivedHeader,
    MemoryReference body)
{
    NOT_IMPLEMENTED(js);
}

bool JSObjectScript::valid() const {
    return (mParent);
}



void JSObjectScript::test() const {
    const HostedObject::SpaceSet& spaces = mParent->spaces();
    assert(spaces.size() == 1);

    SpaceID space = *(spaces.begin());

    Location loc = mParent->getLocation( space );
    loc.setPosition( loc.getPosition() + Vector3<float64>(.5f, .5f, .5f) );
    loc.setOrientation( loc.getOrientation() * Quaternion(Vector3<float32>(0.0f, 0.0f, 1.0f), 3.14159/18.0) );
    loc.setAxisOfRotation( Vector3<float32>(0.0f, 0.0f, 1.0f) );
    loc.setAngularSpeed(3.14159/10.0);
    mParent->setLocation( space, loc );

//    mParent->setVisual(space, Transfer::URI(" http://www.sirikata.com/content/assets/tetra.dae"));
    mParent->setVisualScale(space, Vector3f(1.f, 1.f, 2.f) );

    printf("\n\n\n\n\nI GOT HERE\n\n\n");
    //doing a simple testSendMessage
    bftm_testSendMessageBroadcast("default message");
}



//bftm
//populates the internal addressable object references vector
void JSObjectScript::populateAddressable(Handle<Object>& system_obj )
{

    //loading the vector
    mAddressableList.clear();
    bftm_getAllMessageable(mAddressableList);

    v8::Context::Scope context_scope(mContext);
    v8::Local<v8::Array> arrayObj= v8::Array::New();


    const HostedObject::SpaceSet& spaces = mParent->spaces();
    SpaceID spaceider = *(spaces.begin());

    UUID myUUID = mParent->getObjReference(spaceider).getAsUUID();;


    for (int s=0;s < (int)mAddressableList.size(); ++s)
    {
        Local<Object> tmpObj = mManager->mAddressableTemplate->NewInstance();
        tmpObj->SetInternalField(OREF_OREF_FIELD,External::New(mAddressableList[s]));
        tmpObj->SetInternalField(OREF_JSOBJSCRIPT_FIELD,External::New(this));

        arrayObj->Set(v8::Number::New(s),tmpObj);

        if(mAddressableList[s]->getAsUUID().toString() == myUUID.toString())
            system_obj->Set(v8::String::New(JSSystemNames::ADDRESSABLE_SELF_NAME), tmpObj);


    }
    system_obj->Set(v8::String::New(JSSystemNames::ADDRESSABLE_ARRAY_NAME),arrayObj);

}



int JSObjectScript::getAddressableSize()
{
    return (int)mAddressableList.size();
}



//bftm
//this function sends a message to all functions that proxyObjectManager has an
//ObjectReference for
void JSObjectScript::bftm_testSendMessageBroadcast(const std::string& msgToBCast) const
{
    Sirikata::JS::Protocol::JSMessage js_object_msg;
    Sirikata::JS::Protocol::IJSField js_object_field = js_object_msg.add_fields();

    for (int s=0; s < (int)mAddressableList.size(); ++s)
        sendMessageToEntity(mAddressableList[s],msgToBCast);
}


//bftm
//takes in the index into the mAddressableList (for the object reference) and
//a string that forms the message body.
void JSObjectScript::sendMessageToEntity(int numIndex, const std::string& msgBody) const
{
    sendMessageToEntity(mAddressableList[numIndex],msgBody);
}

void JSObjectScript::sendMessageToEntity(ObjectReference* reffer, const std::string& msgBody) const
{
    const HostedObject::SpaceSet& spaces = mParent->spaces();
    SpaceID spaceider = *(spaces.begin());
    ODP::Endpoint dest (spaceider,*reffer,Services::COMMUNICATION);
    mMessagingPort->send(dest,MemoryReference(msgBody));
}




v8::Handle<v8::Value> JSObjectScript::protectedEval(const String& script_str)
{
    Context::Scope context_scope(mContext);
    v8::HandleScope handle_scope;
    TryCatch try_catch;

    v8::Handle<v8::String> source = v8::String::New(script_str.c_str(), script_str.size());

    // Compile
    //note, because using compile command, will run in the mContext context
    v8::Handle<v8::Script> script = v8::Script::Compile(source);
    if (script.IsEmpty()) {
        v8::String::Utf8Value error(try_catch.Exception());
        std::string msg = std::string("Compile error: ") + std::string(*error);
        JSLOG(error, msg);
        return v8::ThrowException( v8::Exception::Error(v8::String::New(msg.c_str())) );
    }

    // Execute
    v8::Handle<v8::Value> result = script->Run();
    if (result.IsEmpty()) {
        v8::String::Utf8Value error(try_catch.Exception());
        JSLOG(error, "Uncaught exception: " << *error);
        return try_catch.Exception();
    }

    if (!result->IsUndefined()) {
        v8::String::AsciiValue ascii(result);
        JSLOG(info, "Script result: " << *ascii);
    }

    return result;
}

//bftm
void JSObjectScript::bftm_getAllMessageable(std::vector<ObjectReference*>&allAvailableObjectReferences) const
{
    const HostedObject::SpaceSet& spaces = mParent->spaces();
    //which space am I in now
    SpaceID spaceider = *(spaces.begin());

    //get a list of all object references through prox
    //ProxyObjectPtr proxPtr = mParent->getProxyManager(spaceider);
    ObjectHostProxyManager* proxManagerPtr = mParent->bftm_getProxyManager(spaceider);

    //FIX ME: May need to check if get back null ptr.

    //proxManagerPtr->getAllObjectReferences(allAvailableObjectReferences);
    proxManagerPtr->getAllObjectReferences(allAvailableObjectReferences);

    std::cout<<"\n\nBFTM:  this is the number of objects that are messageable:  "<<allAvailableObjectReferences.size()<<"\n\n";

    if (allAvailableObjectReferences.empty())
    {
        printf("\n\nBFTM: No object references available for sending messages");
        return;
    }
}






void ProtectedJSCallback(v8::Handle<v8::Context> ctx, v8::Handle<v8::Object> target, v8::Handle<v8::Function> cb, int argc, v8::Handle<v8::Value> argv[]) {
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(ctx);

    TryCatch try_catch;

    Handle<Value> result;
    if (target->IsNull() || target->IsUndefined())
        result = cb->Call(ctx->Global(), argc, argv);
    else
        result = cb->Call(target, argc, argv);

    if (result.IsEmpty()) {
        // FIXME what should we do with this exception?
        v8::String::Utf8Value error(try_catch.Exception());
        const char* cMsg = ToCString(error);
        std::cout << cMsg << "\n";
	}
}

void ProtectedJSCallback(v8::Handle<v8::Context> ctx, v8::Handle<v8::Object> target, v8::Handle<v8::Function> cb) {
    const int argc =
#ifdef _WIN32
		1
#else
		0
#endif
		;
    Handle<Value> argv[argc] = { };
    ProtectedJSCallback(ctx, target, cb, argc, argv);
}

void JSObjectScript::timeout(const Duration& dur, v8::Persistent<v8::Object>& target, v8::Persistent<v8::Function>& cb) {
    // FIXME using the raw pointer isn't safe
    mParent->getObjectHost()->getSpaceIO()->post(
        dur,
        std::tr1::bind(
            &JSObjectScript::handleTimeout,
            this,
            target,
            cb
        )
    );
}

void JSObjectScript::handleTimeout(v8::Persistent<v8::Object> target, v8::Persistent<v8::Function> cb) {
    ProtectedJSCallback(mContext, target, cb);
}

v8::Handle<v8::Value> JSObjectScript::import(const String& filename) {
    FILE * pFile;
    long lSize;
    char * buffer;
    long result;

    pFile = fopen (filename.c_str(), "r" );
    if (pFile == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Couldn't open file for import.")) );

    fseek (pFile , 0 , SEEK_END);
    lSize = ftell (pFile);
    rewind (pFile);

    std::string contents(lSize, '\0');

    result = fread (&(contents[0]), 1, lSize, pFile);
    fclose (pFile);

    if (result != lSize)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Failure reading file for import.")) );

    return protectedEval(contents);
}


#define FIXME_GET_SPACE() \
    const HostedObject::SpaceSet& spaces = mParent->spaces(); \
    assert(spaces.size() == 1);                               \
    SpaceID space = *(spaces.begin());

// v8::Handle<v8::String> JSObjectScript::getVisual() {
//     FIXME_GET_SPACE();

//     String url_string = mParent->getVisual(space).toString();
//     return v8::String::New( url_string.c_str(), url_string.size() );
// }

// void JSObjectScript::setVisual(v8::Local<v8::Value>& newvis) {
//     // Can/should we do anything about a failure here?
//     if (!newvis->IsString())
//         return;

//     v8::String::Utf8Value newvis_str(newvis);
//     if (! *newvis_str)
//         return; // FIXME failure?
//     Transfer::URI vis_uri(*newvis_str);

//     FIXME_GET_SPACE();
//     mParent->setVisual(space, vis_uri);
// }


v8::Handle<v8::String> JSObjectScript::getVisual(const SpaceID* sID)
{

    String url_string = mParent->getVisual(*sID).toString();
    return v8::String::New( url_string.c_str(), url_string.size() );
}

//FIXME: May want to have an error handler for this function.
void  JSObjectScript::setVisual(const SpaceID* sID, const Transfer::URI* newMesh)
{
    mParent->setVisual(*sID,*newMesh);
}


//FIXME: need to return the right space here.
//lkjs; need to return the visuals for a particular space.;
v8::Handle<v8::Value> JSObjectScript::getVisualScale() {
    FIXME_GET_SPACE();
    return CreateJSResult(mContext, mParent->getVisualScale(space));
}

void JSObjectScript::setVisualScale(v8::Local<v8::Value>& newscale) {
    Handle<Object> scale_obj = ObjectCast(newscale);
    if (!Vec3Validate(scale_obj))
        return;

    Vector3f native_scale(Vec3Extract(scale_obj));
    FIXME_GET_SPACE();
    mParent->setVisualScale(space, native_scale);
}

#define CreateLocationAccessorHandlersWithSpace(PropType, PropName, SubType, SubTypeCast, Validator, Extractor) \
    v8::Handle<v8::Value> JSObjectScript::get##PropName(SpaceID& space) {             \
        /*FIXME_GET_SPACE(); */                                             \
        return CreateJSResult(mContext, mParent->getLocation(space).get##PropName()); \
    }                                                                   \
                                                                        \
    void JSObjectScript::set##PropName(SpaceID& space, v8::Local<v8::Value>& newval) {  \
        Handle<SubType> val_obj = SubTypeCast(newval);                  \
        if (!Validator(val_obj))                                        \
            return;                                                     \
                                                                        \
        PropType native_val(Extractor(val_obj));                        \
                                                                        \
        /* FIXME_GET_SPACE(); */                                           \
        Location loc = mParent->getLocation(space);                     \
        loc.set##PropName( native_val);                                  \
        mParent->setLocation(space, loc);                               \
    } \



//presence version of the access handlers

CreateLocationAccessorHandlersWithSpace(Vector3d, Position, Object, ObjectCast, Vec3Validate, Vec3Extract)
CreateLocationAccessorHandlersWithSpace(Vector3f, Velocity, Object, ObjectCast, Vec3Validate, Vec3Extract)



#define CreateLocationAccessorHandlers(PropType, PropName, SubType, SubTypeCast, Validator, Extractor) \
    v8::Handle<v8::Value> JSObjectScript::get##PropName() {             \
        FIXME_GET_SPACE();                                              \
        return CreateJSResult(mContext, mParent->getLocation(space).get##PropName()); \
    }                                                                   \
                                                                        \
    void JSObjectScript::set##PropName(v8::Local<v8::Value>& newval) {  \
        Handle<SubType> val_obj = SubTypeCast(newval);                  \
        if (!Validator(val_obj))                                        \
            return;                                                     \
                                                                        \
        PropType native_val(Extractor(val_obj));                        \
                                                                        \
        FIXME_GET_SPACE();                                            \
        Location loc = mParent->getLocation(space);                     \
        loc.set##PropName(native_val);                                  \
        mParent->setLocation(space, loc);                               \
    }

#define NOOP_CAST(X) X

CreateLocationAccessorHandlers(Vector3d, Position, Object, ObjectCast, Vec3Validate, Vec3Extract)
CreateLocationAccessorHandlers(Vector3f, Velocity, Object, ObjectCast, Vec3Validate, Vec3Extract)
CreateLocationAccessorHandlers(Quaternion, Orientation, Object, ObjectCast, QuaternionValidate, QuaternionExtract)
CreateLocationAccessorHandlers(Vector3f, AxisOfRotation, Object, ObjectCast, Vec3Validate, Vec3Extract)
CreateLocationAccessorHandlers(double, AngularSpeed, Value, NOOP_CAST, NumericValidate, NumericExtract)

void JSObjectScript::setPositionFunction(const SpaceID* sID, const Vector3d& vec3d)
{
    Location loc = mParent->getLocation(*sID);
    loc.setPosition(vec3d);
}


v8::Handle<v8::Value> JSObjectScript::getPositionFunction(const SpaceID* sID)
{
    Location loc = mParent->getLocation(*sID);
    //FIXME: note that CreateJSResult may not work with vec3, which is what
    //second arg returns
    return CreateJSResult(mContext, mParent->getLocation(*sID).getPosition());
}


JSEventHandler* JSObjectScript::registerHandler(const PatternList& pattern, v8::Persistent<v8::Object>& target, v8::Persistent<v8::Function>& cb, v8::Persistent<v8::Object>& sender)
{
    JSEventHandler* new_handler= new JSEventHandler(pattern, target, cb,sender);


    if ( mHandlingEvent)
    {
        //means that we're in the process of handling an event, and therefore
        //cannot push onto the event handlers list.  instead, add it to another
        //vector, which are additional changes to make after we've tried to
        //match all events.
        mQueuedHandlerEventsAdd.push_back(new_handler);
    }
    else
        mEventHandlers.push_back(new_handler);


    return new_handler;
}


//for debugging
void JSObjectScript::printAllHandlerLocations()
{
    std::cout<<"\nPrinting event handlers for "<<this<<": \n";
    for (int s=0; s < (int) mEventHandlers.size(); ++s)
        std::cout<<"\t"<<mEventHandlers[s];

    std::cout<<"\n\n\n";
}


/*
 * Populates the message properties
 */

v8::Local<v8::Object> JSObjectScript::getMessageSender(const RoutableMessageHeader& msgHeader)
{
    /**
       FIXME: may need to declare a scope here?
    */

  ObjectReference* orp = new ObjectReference(msgHeader.source_object());

  Local<Object> tmpObj = mManager->mAddressableTemplate->NewInstance();
  tmpObj->SetInternalField(OREF_OREF_FIELD,External::New(orp));
  tmpObj->SetInternalField(OREF_JSOBJSCRIPT_FIELD,External::New(this));


  return tmpObj;
}

//just a handler for receiving any message.  for now, not doing any dispatch.
void JSObjectScript::bftm_handleCommunicationMessage(const RoutableMessageHeader& hdr, MemoryReference payload)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext);
    v8::Local<v8::Object> obj = v8::Object::New();

    v8::Local<v8::Object> msgSender = getMessageSender(hdr);
    //try deserialization
    bool deserializeWorks;
    deserializeWorks = JSSerializer::deserializeObject( payload,obj);


    /*
      FIXME: perhaps send a message back saying that deserialization failed
    */
    if (! deserializeWorks)
        return;


    // Checks if matches some handler.  Try to dispatch the message
    bool matchesSomeHandler = false;


    //cannot affect the event handlers when we are executing event handlers.
    mHandlingEvent = true;

    for (int s=0; s < (int) mEventHandlers.size(); ++s)
    {
        if (mEventHandlers[s]->matches(obj,msgSender))
        {
            // Adding support for the knowing the message properties too
            int argc = 2;

            Handle<Value> argv[2] = { obj, msgSender };
            ProtectedJSCallback(mContext, mEventHandlers[s]->target, mEventHandlers[s]->cb, argc, argv);

            matchesSomeHandler = true;
        }
    }


    mHandlingEvent = false;
    flushQueuedHandlerEvents();


    /*
      FIXME: What should I do if the message that I receive does not match any handler?
     */
    if (!matchesSomeHandler)
        std::cout<<"\n\nMessage did not match any files\n\n";

}


//This function takes care of all of the event handling changes that were queued
//while we were trying to match event happenings.
//adds all outstanding changes and then deletes all outstanding in that order.
void JSObjectScript::flushQueuedHandlerEvents()
{
    //Adding
    for (int s=0; s < (int)mQueuedHandlerEventsAdd.size(); ++s)
    {
        //add handlers requested to be added during matching of handlers
        mEventHandlers.push_back(mQueuedHandlerEventsAdd[s]);
    }
    mQueuedHandlerEventsAdd.clear();


    //deleting
    for (int s=0; s < (int)mQueuedHandlerEventsDelete.size(); ++s)
    {
        //remove handlers requested to be deleted during matching of handlers
        removeHandler(mQueuedHandlerEventsDelete[s]);
    }

    for (int s=0; s < (int) mQueuedHandlerEventsDelete.size(); ++s)
    {
        //actually delete the patterns
        //have to do this sort of tortured structure with comparing against
        //nulls in order to prevent deleting something twice (a user may have
        //tried to get rid of this handler multiple times).
        if (mQueuedHandlerEventsDelete[s] != NULL)
        {
            delete mQueuedHandlerEventsDelete[s];
            mQueuedHandlerEventsDelete[s] = NULL;
        }
    }
    mQueuedHandlerEventsDelete.clear();
}



void JSObjectScript::removeHandler(JSEventHandler* toRemove)
{
    JSEventHandlerList::iterator iter = mEventHandlers.begin();
    while (iter != mEventHandlers.end())
    {
        if ((*iter) == toRemove)
            iter = mEventHandlers.erase(iter);
        else
            ++iter;
    }
}

//takes in an event handler, if not currently handling an event, removes the
//handler from the vector and deletes it.  Otherwise, adds the handler for
//removal and deletion later.
void JSObjectScript::deleteHandler(JSEventHandler* toDelete)
{
    if (mHandlingEvent)
    {
        mQueuedHandlerEventsDelete.push_back(toDelete);
        return;
    }

    removeHandler(toDelete);
    delete toDelete;
    toDelete = NULL;
}



void JSObjectScript::handleScriptingMessage(const RoutableMessageHeader& hdr, MemoryReference payload)
{
    // Parse (FIXME we have to parse a RoutableMessageBody here, should just be
    // in "Header")
    RoutableMessageBody body;
    body.ParseFromArray(payload.data(), payload.size());
    Sirikata::Protocol::ScriptingMessage scripting_msg;
    bool parsed = scripting_msg.ParseFromString(body.payload());
    if (!parsed)
    {
        JSLOG(fatal, "Parsing failed.");
        return;
    }

    // Handle all requests
    for(int32 ii = 0; ii < scripting_msg.requests_size(); ii++)
    {
        Sirikata::Protocol::ScriptingRequest req = scripting_msg.requests(ii);
        String script_str = req.body();

        protectedEval(script_str);
    }
}

//This function takes in a jseventhandler, and wraps a javascript object with
//it.  The function is called by registerEventHandler in JSSystem, which returns
//the js object this function creates to the user.
v8::Handle<v8::Object> JSObjectScript::makeEventHandlerObject(JSEventHandler* evHand)
{
    v8::Context::Scope context_scope(mContext);
    v8::HandleScope handle_scope;

    v8::Handle<v8::Object> returner =mManager->mHandlerTemplate->NewInstance();

    returner->SetInternalField(JSHANDLER_JSEVENTHANDLER_FIELD, External::New(evHand));
    returner->SetInternalField(JSHANDLER_JSOBJSCRIPT_FIELD, External::New(this));
    
    return returner;
}





Handle<Object> JSObjectScript::getGlobalObject()
{
  // we really don't need a persistent handle
  //Local handles should work
  return mContext->Global();
}


Handle<Object> JSObjectScript::getSystemObject()
{
  HandleScope handle_scope;
  Local<Object> global_obj = mContext->Global();
  // NOTE: See v8 bug 162 (http://code.google.com/p/v8/issues/detail?id=162)
  // The template actually generates the root objects prototype, not the root
  // itself.
  Handle<Object> global_proto = Handle<Object>::Cast(global_obj->GetPrototype());
  // And we add an internal field to the system object as well to make it
  // easier to find the pointer in different calls. Note that in this case we
  // don't use the prototype -- non-global objects work as we would expect.
  Local<Object> system_obj = Local<Object>::Cast(global_proto->Get(v8::String::New(JSSystemNames::ROOT_OBJECT_NAME)));
  
  Persistent<Object> ret_obj = Persistent<Object>::New(system_obj);
  return ret_obj;
}


void JSObjectScript::updateAddressable()
{
  HandleScope handle_scope;
  Handle<Object> system_obj = getSystemObject();
  populateAddressable(system_obj);
}


//called to build the presences array as well as to build the presence keyword
void JSObjectScript::initializePresences(Handle<Object>& system_obj)
{
    clearAllPresences(system_obj);
    populatePresences(system_obj);
}



//this function grabs all presence-related data, and frees the memory (that
//we're responsible for) associated with each.
void JSObjectScript::clearAllPresences(Handle<Object>& system_obj)
{
    system_obj->Delete(v8::String::New(JSSystemNames::PRESENCES_ARRAY_NAME));

    for (int s=0; s < (int) mPresenceList.size(); ++s)
    {
        delete mPresenceList[s]->sID;
        delete mPresenceList[s];
    }
    mPresenceList.clear();

    if (mPres != NULL)
    {
        delete mPres->sID;
        mPres = NULL;
    }
}


//this function adds the presences array to the system object
void JSObjectScript::populatePresences(Handle<Object>& system_obj)
{
    HandleScope handle_scope;
    HostedObject::SpaceSet spaces = mParent->spaces();
    HostedObject::SpaceSet::const_iterator it = spaces.begin();
    v8::Context::Scope context_scope(mContext);
    v8::Local<v8::Array> arrayObj= v8::Array::New();

    for (int s=0; it != spaces.end(); ++s, ++it)
    {
        Local<Object> tmpObj = mManager->mPresenceTemplate->NewInstance();
        SpaceID* space_id = new SpaceID(*it);


        JSPresenceStruct* presToAdd = new JSPresenceStruct;
        presToAdd->sID = space_id;
        presToAdd->jsObjScript = this;
        mPresenceList.push_back(presToAdd);
        
        tmpObj->SetInternalField(PRESENCE_FIELD,External::New(presToAdd));
        arrayObj->Set(v8::Number::New(s),tmpObj);

    }

    system_obj->Set(v8::String::New(JSSystemNames::PRESENCES_ARRAY_NAME),arrayObj);
}





//this function can be called to re-initialize the system object's state
void JSObjectScript::populateSystemObject(Handle<Object>& system_obj)
{
   HandleScope handle_scope;
   //takes care of the addressable array in sys.
   
   system_obj->SetInternalField(0, External::New(this));

   
   //FIXME: May need an initialize addressable
   populateAddressable(system_obj);
   
   initializePresences(system_obj);
}


void JSObjectScript::attachScript(const String& script_name)
{
  import(script_name);  
}

void JSObjectScript::create_presence(const SpaceID& new_space)
{
 
  const HostedObject::SpaceSet& spaces = mParent->spaces();
  
  SpaceID spaceider = *(spaces.begin());
  const BoundingSphere3f& bs = BoundingSphere3f(Vector3f(0, 0, 0), 1);

  mParent->connectToSpace(new_space, mParent->getSharedPtr(), mParent->getLocation(spaceider),bs, mParent->getUUID());

  //FIXME: will need to add this presence to the presences vector.
  //but only want to do so when the function has succeeded.
  
}



} // namespace JS
} // namespace Sirikata