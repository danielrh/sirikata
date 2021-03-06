/*  Sirikata Subscription and Broadcasting System -- Broadcast Services
 *  Broadcast.cpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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

#include <subscription/Platform.hpp>
#include "subscription/Broadcast.hpp"
#include <boost/thread.hpp>
#include "network/Stream.hpp"
#include "network/StreamFactory.hpp"
#include "Subscription_Subscription.pbj.hpp"
#include "options/Options.hpp"
namespace Sirikata { namespace Subscription {

class Broadcast::UniqueLock: public boost::mutex {};

Broadcast::BroadcastStream::BroadcastStream(const std::tr1::shared_ptr<Network::Stream>&tls,
                                            Network::Stream*stream) :mTopLevelStream(tls),mStream(stream){}

Broadcast::BroadcastStream::~BroadcastStream(){
    delete mStream;
}
Broadcast::Broadcast(Network::IOService*service, const String&options):mIOService(service) {
    mUniqueLock=new UniqueLock;
    OptionValue*protocolOptions;
    InitializeClassOptions ico("broadcast",this,
                               protocolOptions=new OptionValue("protocols","",OptionValueType<std::map<std::string,std::string> >(),"passes options into protocol specific libraries like \"tcpsst:{--send-buffer-size=1440 --parallel-sockets=1},udp:{--send-buffer-size=1500}\""),
                           NULL);
    OptionSet::getOptions("broadcast",this)->parse(options);
    {
        std::map<std::string,std::string> *options=&protocolOptions->as<std::map<std::string,std::string> > ();
        for (std::map<std::string,std::string>::iterator i=options->begin(),ie=options->end();i!=ie;++i) {
            mProtocolOptions[i->first]=Network::StreamFactory::getSingleton().getOptionParser(i->first)(i->second);
        }
    }
}
class Broadcast::BroadcastStreamCallbacks {
public:
    static void setBroadcastStreamCallbacks(Broadcast::BroadcastStream*thus,
                                            const std::tr1::function<void(Broadcast::BroadcastStream*,
                                                                          Network::Stream::ConnectionStatus,
                                                                          const std::string&reason)>& cb,
                                            Network::Stream*stream,
                                            Network::Stream::SetCallbacks&setCallbacks) {
        thus->mStream=stream;
        setCallbacks(std::tr1::bind(cb,thus,_1,_2),
                     &Network::Stream::ignoreReceivedCallback,
                     &Network::Stream::ignoreReadySendCallback);

    }
    static void setBroadcastStreamCallbacksShared(const std::tr1::shared_ptr<Broadcast::BroadcastStream>&thus,
                                                  const std::tr1::function<void(const std::tr1::weak_ptr<Broadcast::BroadcastStream>&,
                                                                                Network::Stream::ConnectionStatus,
                                                                                const std::string&reason)>& cb,
                                                  Network::Stream*stream,
                                                  Network::Stream::SetCallbacks&setCallbacks){
        std::tr1::weak_ptr<Broadcast::BroadcastStream> bs(thus);
        thus->mStream=stream;
        setCallbacks(std::tr1::bind(cb,bs,_1,_2),
                     &Network::Stream::ignoreReceivedCallback,
                     &Network::Stream::ignoreReadySendCallback);

    }
};
void Broadcast::initiateHandshake(Broadcast::BroadcastStream*stream,const Network::Address&addy,const UUID &name) {
    Protocol::Broadcast message;
    message.set_broadcast_name(name);
    String str;
    if (message.SerializeToString(&str)) {
        (*stream)->send(MemoryReference(str),Network::ReliableOrdered);
        //FIXME
    }else {
        SILOG(broadcast,error,"Cannot send memory reference to UUID "<<name.toString());
    }
}
std::tr1::shared_ptr<Broadcast::BroadcastStream> Broadcast::establishSharedBroadcast(const Network::Address&addy,
                                                                               const UUID&name,
                                                                               const std::tr1::function<void(const std::tr1::weak_ptr<Broadcast::BroadcastStream>&,
                                                                                                             Network::Stream::ConnectionStatus,
                                                                                                             const std::string&reason)>& cb) {
    std::tr1::shared_ptr<Broadcast::BroadcastStream> retval;
    Network::Stream*newBroadcastStream=NULL;
    while(newBroadcastStream==NULL) {
        boost::lock_guard<boost::mutex>lok(*mUniqueLock);
        std::tr1::weak_ptr<Network::Stream>*weak_topLevelStream=&mTopLevelStreams[addy];
        std::tr1::shared_ptr<Network::Stream> topLevelStream;
        if ((topLevelStream=weak_topLevelStream->lock())) {
        }else{
            String protocol(addy.getProtocol());
            if (protocol.empty()) {
                protocol=Network::StreamFactory::getSingleton().getDefault();
            }
            OptionSet**options=&mProtocolOptions[protocol];
            if (*options==NULL) {
                *options=Network::StreamFactory::getSingleton().getOptionParser(protocol)(String());
            }

            std::tr1::shared_ptr<Network::Stream> tlstemp(Network::StreamFactory::getSingleton().getConstructor(protocol)(mIOService,*options));
            (topLevelStream=tlstemp)->connect(addy,
                                              &Network::Stream::ignoreSubstreamCallback,
                                              &Network::Stream::ignoreConnectionCallback,
                                              &Network::Stream::ignoreReceivedCallback,
                                              &Network::Stream::ignoreReadySendCallback);
            *weak_topLevelStream=tlstemp;
        }
        std::tr1::shared_ptr<Broadcast::BroadcastStream> bs(new BroadcastStream(topLevelStream,NULL));
        std::tr1::shared_ptr<Broadcast::BroadcastStream> weak_bs(bs);
        retval=bs;
        newBroadcastStream=topLevelStream->clone(std::tr1::bind(&BroadcastStreamCallbacks::setBroadcastStreamCallbacksShared,
                                                                weak_bs,
                                                                cb,
                                                                _1,
                                                                _2));
        if (newBroadcastStream){
            *weak_topLevelStream=topLevelStream;
        }else {
            if (mTopLevelStreams.find(addy)!=mTopLevelStreams.end()) {
                mTopLevelStreams.erase(mTopLevelStreams.find(addy));
            }
            SILOG(broadcast,warning,"Toplevel stream failed to clone for address "<<addy.getHostName()<<':'<<addy.getService());
        }
    }
    if (retval)
        initiateHandshake(&*retval,addy,name);
    return retval;
}
Broadcast::BroadcastStream *Broadcast::establishBroadcast(const Network::Address&addy,
                                                          const UUID&name,
                                                          const std::tr1::function<void(BroadcastStream*,
                                                                                        Network::Stream::ConnectionStatus,
                                                                                        const std::string&reason)>& cb) {
    Broadcast::BroadcastStream * retval=NULL;
    Network::Stream*newBroadcastStream=NULL;

    while(newBroadcastStream==NULL) {
        boost::lock_guard<boost::mutex>lok(*mUniqueLock);
        std::tr1::weak_ptr<Network::Stream>*weak_topLevelStream=&mTopLevelStreams[addy];
        std::tr1::shared_ptr<Network::Stream> topLevelStream;
        if ((topLevelStream=weak_topLevelStream->lock())) {
        }else{

            String protocol(addy.getProtocol());
            if (protocol.empty()) {
                protocol=Network::StreamFactory::getSingleton().getDefault();
            }
            OptionSet**options=&mProtocolOptions[protocol];
            if (*options==NULL) {
                *options=Network::StreamFactory::getSingleton().getOptionParser(protocol)(String());
            }
            std::tr1::shared_ptr<Network::Stream> tlstemp(Network::StreamFactory::getSingleton().getConstructor(protocol)(mIOService,*options));
            (topLevelStream=tlstemp)->connect(addy,&Network::Stream::ignoreSubstreamCallback,&Network::Stream::ignoreConnectionCallback,&Network::Stream::ignoreReceivedCallback,&Network::Stream::ignoreReadySendCallback);
            *weak_topLevelStream=tlstemp;
        }
        newBroadcastStream=topLevelStream->clone(std::tr1::bind(&BroadcastStreamCallbacks::setBroadcastStreamCallbacks,
                                                                retval=new BroadcastStream(topLevelStream,NULL),
                                                                cb,
                                                                _1,
                                                                _2));
        if (newBroadcastStream){
            *weak_topLevelStream=topLevelStream;
        }else {
            if (mTopLevelStreams.find(addy)!=mTopLevelStreams.end()) {
                mTopLevelStreams.erase(mTopLevelStreams.find(addy));
            }
            SILOG(broadcast,warning,"Toplevel stream failed to clone for address "<<addy.getHostName()<<':'<<addy.getService());
        }

    }
    initiateHandshake(retval,addy,name);
    return retval;
}

Broadcast::~Broadcast() {
    delete mUniqueLock;
}


} }
