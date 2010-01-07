
#include "util/Standard.hh"
#include "util/UUID.hpp"
#include "OSeg.hpp"
#include "SpaceNode.hpp"
#include "Generator.hpp"
#include "OracleMessageQueue.hpp"
#include "OracleOHMessageQueue.hpp"
#include "Random.hpp"
using namespace Sirikata;
using namespace Sirikata::QueueBench;
void generateSpaceServers(const BoundingBox3d3f&bounds,int num,int width, int rnwidth) {
    std::vector<UUID> servers;
    int rnratio=width/rnwidth;
    SpaceNode* currn=NULL;
    SpaceNode* prevrn=NULL;
    for (int i=0;i<width;++i) {
        for (int j=0;j<width;++j) {
            if (i%rnratio==0&&j%rnratio==0) {
                servers.push_back((currn=new SpaceNode(BoundingBox3d3f(bounds.min()+Vector3d(bounds.across().x*(i/(double)rnwidth),bounds.across().y*(j/(double)rnwidth),0.0),bounds.min()+Vector3d(bounds.across().x*((i+1)/(double)rnwidth),bounds.across().y*((j+1)/(double)rnwidth),bounds.across().z)),NULL))->id());
                if (!prevrn) prevrn=currn;
                //only uncomment if you want to flatten heirarchy currn=prevrn;
            }
            servers.push_back((new SpaceNode(BoundingBox3d3f(bounds.min()+Vector3d(bounds.across().x*(i/(double)width),bounds.across().y*(j/(double)width),0.0),bounds.min()+Vector3d(bounds.across().x*((i+1)/(double)width),bounds.across().y*((j+1)/(double)width),bounds.across().z)),currn))->id());
            
        }
    }
    for (int i=width*width;i<num;++i) {
        typedef boost::uniform_int<> distribution_type;
        typedef boost::variate_generator<base_generator_type&, distribution_type> gen_type;
        gen_type gen(generator, distribution_type(0, servers.size()-1));

        servers.push_back(gSpaceNodes[servers[gen()]]->split()->id());
    }

}

double randomObjectSize() {
    boost::uniform_real<> uni_dist(0,1);
    boost::variate_generator<base_generator_type&, boost::uniform_real<> > uni(generator, uni_dist);

    double size= uni();
    size*=size;
    size*=size;
    return size;
}
void generateObjects(int nss, int nobj,const std::vector<UUID> &objectHosts) {
    for(std::tr1::unordered_map<UUID,SpaceNode*,UUID::Hasher>::iterator i=gSpaceNodes.begin(),ie=gSpaceNodes.end();i!=ie;++i) {
        if (i->second->hasParent()) {//only connect objects to leaf
            for (int j=0;j<(int)(nobj/nss);++j) {
                ObjectData od;
                od.location=i->second->randomLocation();
                od.radialSize=randomObjectSize();
                od.spaceServerNode=i->second->id();
                od.objectHost=objectHosts[rand()%objectHosts.size()];
                oSeg.insert(pseudorandomUUID(),od);
            }
        }
    }
}
void generateObjectHosts(int nssv, int noh, int nobj, bool separateObjectStreams,bool distanceKnowledge,bool remoteRadiusKnowledge,bool localRadiusKnowledge) {
    std::vector<UUID> ohs;
    for (int i=0;i<noh;++i) {
        ohs.push_back((new ObjectHost(separateObjectStreams,distanceKnowledge,remoteRadiusKnowledge,localRadiusKnowledge))->id());
    }
    generateObjects(nssv,nobj,ohs);
}
int main() {
    int nobj=8192;
    int nssv=128;
    int rnwidth=3;
    int noh=512;
    bool distanceKnowledge=false;
    bool remoteRadiusKnowledge=true;
    bool localRadiusKnowledge=true;
    int ohQueueSize=128;
    int spaceQueueSize=128;
    int toplevelgridwidth=6;
    int nmsg=/*1024*1024;*/ohQueueSize*noh;


    bool separateObjectStreams=true;
    BoundingBox3d3f bounds(Vector3d::nil(),Vector3d(100000.,100000.,100));
    generateSpaceServers(bounds, nssv,toplevelgridwidth,rnwidth);
    generateObjectHosts(nssv,noh,nobj,separateObjectStreams,distanceKnowledge,remoteRadiusKnowledge,localRadiusKnowledge);

    Generator *rmg=new RandomMessageGenerator;
    std::vector<Message>messages(nmsg);
    for (int i=0;i<nmsg;++i) {
        messages[i]=rmg->generate(oSeg.random());
    }


    std::map<double,int> finalMessageOrder;
    std::map<double,int> oracleMessageOrder;
    if (true) {
        OracleOHMessageQueue omq;
        for (int i=0;i<nmsg;++i) {
            omq.insertMessage(messages[i]);
        }
        Message msg;
        int i=0;
        double lastPriority=1.e38;
        while(omq.popMessage(msg)) {
            double priority=standardfalloff(msg.source,msg.dest);
            if (priority>lastPriority) {
                //printf ("Priority Error %.40f vs %.40f\n",priority,lastPriority);
                
            }
            double delta=1.e-38;
            while (priority==lastPriority) {
                priority-=delta;
                delta*=2;
            }

            lastPriority=priority;
            oracleMessageOrder[-priority]=i;
            //std::cout<<gPriority(msg.source,msg.dest)<<std::endl;
            ++i;
        }
    }
    if (true) {
        for (int i=0;i<nmsg;++i) {
            gObjectHosts[oSeg[messages[i].source]->objectHost]->insertMessage(messages[i]);
        }
        for (SpaceNodeMap::iterator i=gSpaceNodes.begin(),ie=gSpaceNodes.end();i!=ie;++i) {
            if (i->second->hasParent()) {
                i->second->pullFromOH(ohQueueSize);
            }else {
                i->second->pullFromSpaces(ohQueueSize);
            }
        }
        for (SpaceNodeMap::iterator i=gSpaceNodes.begin(),ie=gSpaceNodes.end();i!=ie;++i) {
            if (i->second->hasParent()){
                i->second->pullFromSpaces(spaceQueueSize);
            }else{
                i->second->pullFromRNs(spaceQueueSize);
            }
        }
        for (int i=0;i<nmsg;) {
            Message msg;
            bool nonepulled=true;
            for (std::tr1::unordered_map<UUID,ObjectHost*,UUID::Hasher>::iterator j=gObjectHosts.begin(),je=gObjectHosts.end();
                 j!=je;
                 ++j) {
                if (j->second->pull(msg)) {
                    finalMessageOrder[-standardfalloff(msg.source,msg.dest)]=i;
                    nonepulled=false;
                    ++i;
                }
            }
            {
                for (SpaceNodeMap::iterator i=gSpaceNodes.begin(),ie=gSpaceNodes.end();i!=ie;++i) {
                    if (i->second->hasParent()) {
                        i->second->pullFromOH(ohQueueSize);
                    }else {
                        i->second->pullFromSpaces(ohQueueSize);
                    }
                }
                for (SpaceNodeMap::iterator i=gSpaceNodes.begin(),ie=gSpaceNodes.end();i!=ie;++i) {
                    if (i->second->hasParent()) {
                        i->second->pullFromSpaces(spaceQueueSize);
                    }else{
                        i->second->pullFromRNs(spaceQueueSize);
                    }
                }
            }
            if (nonepulled&&i<nmsg) {
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
        }
    }
    int64 totalError=0;
    std::map<double,int>::iterator iter=finalMessageOrder.begin();
    for (size_t i=0;i<finalMessageOrder.size();++i,++iter) {
        int64 diff=i-iter->second;
        if (oracleMessageOrder.find(iter->first)!=oracleMessageOrder.end()) {
            diff=oracleMessageOrder[iter->first]-iter->second;
        }else {
            std::cout<<"Oracle failed to deliver message "<<iter->first<<"'\n";
        }
        if (diff) {
            //std::cout<<"Diffd "<<diff<<"'\n";
        }
        if (diff<0) diff=-diff;
        totalError+=diff;
    }
    std::cout<<"Total error: "<<totalError<<'\n';
    return 0;
}
