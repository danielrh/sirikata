
#include "util/Standard.hh"
#include "util/UUID.hpp"
#include "OSeg.hpp"
#include "SpaceNode.hpp"
#include "Generator.hpp"
#include "OracleMessageQueue.hpp"
#include "OracleOHMessageQueue.hpp"
#include "Random.hpp"
#include "Evaluation.hpp"
#include "options/Options.hpp"
#include <cmath>
using namespace Sirikata;
using namespace Sirikata::QueueBench;

void generateSpaceServers(const BoundingBox3d3f&bounds,int num,int width, int rnwidth, ObjectKnowledgeDescription spaceKnowledge, ObjectKnowledgeDescription spaceOutputKnowledge, ObjectKnowledgeDescription rnKnowledge, bool resort_space_message_queues, bool resort_space_output_message_queues,bool resort_rn_message_queues) {
    std::vector<UUID> servers;
    int rnratio=width/rnwidth;
    SpaceNode* currn=NULL;
    SpaceNode* prevrn=NULL;
    for (int i=0;i<width;++i) {
        for (int j=0;j<width;++j) {
            SpaceNode*hypotheticalParent=new SpaceNode(BoundingBox3d3f(bounds.min()+Vector3d(bounds.across().x*(i/(double)rnwidth),bounds.across().y*(j/(double)rnwidth),0.0),bounds.min()+Vector3d(bounds.across().x*((i+1)/(double)rnwidth),bounds.across().y*((j+1)/(double)rnwidth),bounds.across().z)),NULL,rnKnowledge,rnKnowledge,resort_rn_message_queues,resort_rn_message_queues);
            if (i%rnratio==0&&j%rnratio==0) {
                servers.push_back((currn=hypotheticalParent)->id());
                if (!prevrn) prevrn=currn;
                //only uncomment if you want to flatten heirarchy currn=prevrn;
            }
            servers.push_back((new SpaceNode(BoundingBox3d3f(bounds.min()+Vector3d(bounds.across().x*(i/(double)width),bounds.across().y*(j/(double)width),0.0),bounds.min()+Vector3d(bounds.across().x*((i+1)/(double)width),bounds.across().y*((j+1)/(double)width),bounds.across().z)),currn,spaceKnowledge,spaceOutputKnowledge,resort_space_message_queues,resort_space_output_message_queues))->id());
            
        }
    }
    for (int i=width*width;i<num;++i) {
        typedef boost::uniform_int<> distribution_type;
        typedef boost::variate_generator<base_generator_type&, distribution_type> gen_type;
        gen_type gen(generator, distribution_type(0, servers.size()-1));

        servers.push_back(gSpaceNodes[servers[gen()]]->split()->id());
    }

}
double randomObjectSize(double largeObjectSize) {
#if 1
    typedef boost::uniform_int<> distribution_type;
    typedef boost::variate_generator<base_generator_type&, distribution_type> gen_type;
    static gen_type gen(generator, distribution_type(0, 1));
    if (gen()) {
        return largeObjectSize;
    }
    return 1;
    
#else
    boost::uniform_real<> uni_dist(0,1);
    boost::variate_generator<base_generator_type&, boost::uniform_real<> > uni(generator, uni_dist);

    double size= /*sqrt*/(uni());
    return size;
    size*=size;
    size*=size;
    return size;
#endif
}
void generateObjects(int nss, int nobj,const std::vector<UUID> &objectHosts, double largeObjectSize) {
    for(std::tr1::unordered_map<UUID,SpaceNode*,UUID::Hasher>::iterator i=gSpaceNodes.begin(),ie=gSpaceNodes.end();i!=ie;++i) {
        if (i->second->hasParent()) {//only connect objects to leaf
            for (int j=0;j<(int)(nobj/nss);++j) {
                ObjectData od;
                od.location=i->second->randomLocation();
                od.radius=randomObjectSize(largeObjectSize);
                od.spaceServerNode=i->second->id();
                od.objectHost=objectHosts[rand()%objectHosts.size()];
                oSeg.insert(pseudorandomUUID(),od);
            }
        }
    }
}
void generateObjectHosts(int nssv, int noh, int nobj, double largeObjectSize, bool separateObjectStreams, const ObjectKnowledgeDescription knowledge, bool objectMessageQueueIsFair) {
    std::vector<UUID> ohs;
    for (int i=0;i<noh;++i) {
        ohs.push_back((new ObjectHost(separateObjectStreams,knowledge,objectMessageQueueIsFair))->id());
    }
    resetPseudorandomUUID(3);
    generateObjects(nssv,nobj,ohs, largeObjectSize);
}
MessagePriorityMap finalMessageOrder=MessagePriorityMap(MessagePriority(xstandardfalloff));
MessagePriorityMap oracleMessageOrder=MessagePriorityMap(MessagePriority(xstandardfalloff));
std::vector<Message>messages=std::vector<Message>(0);
int messageCount=0;
int gOhQueueSize;//var read in main options
int gSpaceQueueSize;//var read in main options
bool gMeasureOracle;
bool loop() {

    for (SpaceNodeMap::iterator i=gSpaceNodes.begin(),ie=gSpaceNodes.end();i!=ie;++i) {
        if (i->second->hasParent()) {
            i->second->pullFromOH(gOhQueueSize);
        }else {
            i->second->pullFromSpaces(gOhQueueSize);
        }
    }

    for (SpaceNodeMap::iterator i=gSpaceNodes.begin(),ie=gSpaceNodes.end();i!=ie;++i) {
        if (i->second->hasParent()) {
            i->second->pullFromSpaces(gSpaceQueueSize);
        }else{
            i->second->pullFromRNs(gSpaceQueueSize);
        }
    }

    bool nonepulled=true;
    for (std::tr1::unordered_map<UUID,ObjectHost*,UUID::Hasher>::iterator j=gObjectHosts.begin(),je=gObjectHosts.end();
         j!=je;
         ++j) {
        Message msg;
        if (j->second->pull(msg)) {
            finalMessageOrder.insert(MessagePriorityMap::value_type(msg,messageCount--));
            nonepulled=false;           
        }
    }
    if (nonepulled&&messageCount>0) {
        for (SpaceNodeMap::iterator i=gSpaceNodes.begin(),ie=gSpaceNodes.end();i!=ie;++i) {
            size_t a,b,c,d;
            a=i->second->waitingMessagesOH();
            b=i->second->waitingMessagesSpace();
            c=i->second->waitingMessagesRN();
            d=i->second->waitingMessagesChild();
            if (a||b||c||d) {
                std::cerr<<"q "<<a<<' '<<b<<' '<<c<< ' '<<d<<'\n';
            }
        }
        
        for (std::tr1::unordered_map<UUID,ObjectHost*,UUID::Hasher>::iterator i=gObjectHosts.begin(),ie=gObjectHosts.end();i!=ie;++i) {
            size_t a=i->second->objectMessageQueueSize();
            if (a) {
                std::cerr<<"h "<<a<<'\n';
            }
        }
        static bool said=false;
        if (!said) {
            std::cerr<<"Fatal error"<<'\n';
            said=true;
        }
        
    }
    if(messageCount<=0) {

        evaluateError(messages,gMeasureOracle?oracleMessageOrder:finalMessageOrder,oracleMessageOrder,100,false);
        return false;
    }
    return true;
}
class FuzzyOptionValue {public:
    static Any lexical_cast(const std::string &value){
        ObjectKnowledgeDescription::Fuzzy retval=ObjectKnowledgeDescription::NONE;
        if (value.size()){
            if (value[0]=='T'||value[0]=='t'||value[0]=='1'||value[0]=='Y'||value[0]=='y')
                retval=ObjectKnowledgeDescription::FULL;
        }
        if (value=="fuzzy") {
            retval=ObjectKnowledgeDescription::FUZZY;
        }
        return retval;
    }
};

int main(int argc, char**argv) {
    OptionValue *nobj=new OptionValue("num-objects","8192",OptionValueType<int>(),"How many objects to be simulated");
    OptionValue* noh=new OptionValue("num-oh","512",OptionValueType<int>(),"How many object host computers are instantiated");
    OptionValue *nssv=new OptionValue("num-spaces","128",OptionValueType<int>(),"How many space nodes are instantiated");
    OptionValue *rnwidth=new OptionValue("num-regions","1",OptionValueType<int>(),"How many region nodes across is the cube");
    OptionValue *largeObjectSize=new OptionValue("max-radius","8",OptionValueType<double>(),"biggest object radius (half are big, half are small)");

    OptionValue *distanceKnowledge=new OptionValue("oh-distance-knowledge","false",OptionValueType<bool>(),"Does OH know destination objects' distances");
    OptionValue *remoteRadiusKnowledge=new OptionValue("oh-remote-radius-knowledge","fuzzy",FuzzyOptionValue(),"Does the OH know about remote object radius");
    OptionValue *localRadiusKnowledge=new OptionValue("oh-local-radius-knowledge","true",FuzzyOptionValue(),"Does the OH know about its own objects' object radius");

    OptionValue *spaceDistanceKnowledge=new OptionValue("space-distance-knowledge","true",OptionValueType<bool>(),"Does Space know destination objects' distances");
    OptionValue *spaceRemoteRadiusKnowledge=new OptionValue("space-remote-radius-knowledge","fuzzy",FuzzyOptionValue(),"Does the Space know about remote object radius");
    OptionValue *spaceLocalRadiusKnowledge=new OptionValue("space-local-radius-knowledge","fuzzy",FuzzyOptionValue(),"Does the Space know about its own objects' object radius");

    OptionValue *spaceOutputDistanceKnowledge=new OptionValue("space-output-distance-knowledge","false",OptionValueType<bool>(),"Does Space know destination objects' distances");
    OptionValue *spaceOutputRemoteRadiusKnowledge=new OptionValue("space-output-remote-radius-knowledge","fuzzy",FuzzyOptionValue(),"Does the Space know about remote object radius");
    OptionValue *spaceOutputLocalRadiusKnowledge=new OptionValue("space-output-local-radius-knowledge","fuzzy",FuzzyOptionValue(),"Does the Space know about its own objects' object radius");

    OptionValue *rnDistanceKnowledge=new OptionValue("rn-distance-knowledge","false",OptionValueType<bool>(),"Does RN know destination objects' distances");
    OptionValue *rnRemoteRadiusKnowledge=new OptionValue("rn-remote-radius-knowledge","fuzzy",FuzzyOptionValue(),"Does the RN know about remote object radius");
    OptionValue *rnLocalRadiusKnowledge=new OptionValue("rn-local-radius-knowledge","fuzzy",FuzzyOptionValue(),"Does the RN know about its own objects' object radius");

    
    OptionValue *ohQueueSize=new OptionValue("oh-queue-size","128",OptionValueType<int>(),"How large is the space's message queue from the OH");
    OptionValue *spaceQueueSize=new OptionValue("space-queue-size","128",OptionValueType<int>(),"How large is the message queue to other Space Servers");

    OptionValue *toplevelgridwidth=new OptionValue("gridwidth","6",OptionValueType<int>(),"How many servers across/down is the whole simulated world");
    
    OptionValue *objectMessageQueueIsFair=new OptionValue("oh-fair-message-queue","false",OptionValueType<bool>(),"Does the OH presort its outgoing messages by priority");
    OptionValue *space_message_queue_is_fair=new OptionValue("space-fair-message-queue","true",OptionValueType<bool>(),"Does the OH presort its outgoing messages by priority");
    OptionValue *space_output_message_queue_is_fair=new OptionValue("space-output-fair-message-queue","false",OptionValueType<bool>(),"Does the OH presort its outgoing messages by priority");
    OptionValue *rn_message_queue_is_fair=new OptionValue("rn-fair-message-queue","false",OptionValueType<bool>(),"Does the OH presort its outgoing messages by priority");
    OptionValue *separateObjectStreams=new OptionValue("separate-object-streams","true",OptionValueType<bool>(),"Does the Object Host have separate streams per object (to be prioritized individually)");
    OptionValue *measureOracle=new OptionValue("oracle","false",OptionValueType<bool>(),"Should we measure direct UDP communication between objects");    
    
    
    OptionValue *nmsg=new OptionValue("num-messages","1048576",OptionValueType<int>(),"How many messages are injected into the system");
    OptionValue* fractionOfMessagesMeasured=new OptionValue("fraction-messages-received",".0625",OptionValueType<double>(),"What fraction of messages received before simulation is terminated");

    BoundingBox3d3f bounds(Vector3d::nil(),Vector3d(100000.,100000.,100));

    InitializeGlobalOptions gbo("",
                                nobj,noh,nssv,rnwidth,largeObjectSize,distanceKnowledge,remoteRadiusKnowledge,localRadiusKnowledge,spaceDistanceKnowledge,spaceRemoteRadiusKnowledge,spaceLocalRadiusKnowledge,spaceOutputDistanceKnowledge,spaceOutputRemoteRadiusKnowledge,spaceOutputLocalRadiusKnowledge,rnDistanceKnowledge,rnRemoteRadiusKnowledge,rnLocalRadiusKnowledge,ohQueueSize,spaceQueueSize,toplevelgridwidth,objectMessageQueueIsFair,space_message_queue_is_fair,space_output_message_queue_is_fair,separateObjectStreams,measureOracle,nmsg,fractionOfMessagesMeasured,rn_message_queue_is_fair
                                ,NULL);
    OptionSet::getOptions("")->parse(argc,argv);    


    gMeasureOracle=measureOracle->as<bool>();
    messageCount=(int)(nmsg->as<int>()*fractionOfMessagesMeasured->as<double>());
    gOhQueueSize=ohQueueSize->as<int>();
    gSpaceQueueSize=spaceQueueSize->as<int>();
    resetPseudorandomUUID(1);

     ObjectKnowledgeDescription spaceKnowledge;
     spaceKnowledge.distanceKnowledge=spaceDistanceKnowledge->as<bool>();
     spaceKnowledge.remoteRadiusKnowledge=spaceRemoteRadiusKnowledge->as<ObjectKnowledgeDescription::Fuzzy>();
     spaceKnowledge.localRadiusKnowledge=spaceLocalRadiusKnowledge->as<ObjectKnowledgeDescription::Fuzzy>();
     ObjectKnowledgeDescription spaceOutputKnowledge;
     spaceOutputKnowledge.distanceKnowledge=spaceOutputDistanceKnowledge->as<bool>();
     spaceOutputKnowledge.remoteRadiusKnowledge=spaceOutputRemoteRadiusKnowledge->as<ObjectKnowledgeDescription::Fuzzy>();
     spaceOutputKnowledge.localRadiusKnowledge=spaceOutputLocalRadiusKnowledge->as<ObjectKnowledgeDescription::Fuzzy>();
     ObjectKnowledgeDescription rnKnowledge;
     rnKnowledge.distanceKnowledge=rnDistanceKnowledge->as<bool>();
     rnKnowledge.remoteRadiusKnowledge=rnRemoteRadiusKnowledge->as<ObjectKnowledgeDescription::Fuzzy>();
     rnKnowledge.localRadiusKnowledge=rnLocalRadiusKnowledge->as<ObjectKnowledgeDescription::Fuzzy>();
     generateSpaceServers(bounds, nssv->as<int>(),toplevelgridwidth->as<int>(),(int)std::floor(sqrt(rnwidth->as<int>())+.5),spaceKnowledge,spaceOutputKnowledge,rnKnowledge,space_message_queue_is_fair->as<bool>(),space_output_message_queue_is_fair->as<bool>(),rn_message_queue_is_fair->as<bool>());
     resetPseudorandomUUID(2);
     ObjectKnowledgeDescription ohKnowledge;
     ohKnowledge.distanceKnowledge=distanceKnowledge->as<bool>();
     ohKnowledge.remoteRadiusKnowledge=remoteRadiusKnowledge->as<ObjectKnowledgeDescription::Fuzzy>();
     ohKnowledge.localRadiusKnowledge=localRadiusKnowledge->as<ObjectKnowledgeDescription::Fuzzy>();
     generateObjectHosts(nssv->as<int>(),noh->as<int>(),nobj->as<int>(),largeObjectSize->as<double>(), separateObjectStreams->as<bool>(),ohKnowledge,objectMessageQueueIsFair->as<bool>());
     resetPseudorandomUUID(4);
     Generator *rmg=new RandomMessageGenerator;
     messages.resize(nmsg->as<int>());
     for (size_t i=0;i<messages.size();++i) {
         messages[i]=rmg->generate(oSeg.random());
     }


     if (true) {
         OracleOHMessageQueue omq(spaceQueueSize->as<int>());
         for (size_t i=0;i<messages.size();++i) {
             omq.insertMessage(messages[i]);
         }
         Message msg;
         int i=0;
         double lastPriority=1.e38;
         for (int index=0;index<messageCount&&omq.popMessage(msg);++index) {
             double priority=xstandardfalloff(msg.source,msg.dest);
            if (priority>lastPriority) {
                //printf ("Priority Error %.40f vs %.40f\n",priority,lastPriority);
                
            }
            double delta=1.e-38;
            while (priority==lastPriority) {
                priority-=delta;
                delta*=2;
            }

            lastPriority=priority;
            oracleMessageOrder.insert(MessagePriorityMap::value_type(msg,i));
            //std::cout<<gPriority(msg.source,msg.dest)<<std::endl;
            ++i;
        }
    }
    for (size_t i=0;i<messages.size();++i) {
        gObjectHosts[oSeg[messages[i].source]->objectHost]->insertMessage(messages[i]);
    }
    
    while(loop()) {

    }
    return 0;
}
