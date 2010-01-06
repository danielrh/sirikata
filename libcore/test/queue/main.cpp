
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
void generateSpaceServers(const BoundingBox3d3f&bounds,int num,int width) {
    std::vector<UUID> servers;
    for (int i=0;i<width;++i) {
        for (int j=0;j<width;++j) {
            servers.push_back((new SpaceNode(BoundingBox3d3f(bounds.min()+Vector3d(bounds.across().x*(i/(double)width),bounds.across().y*(j/(double)width),0.0),bounds.min()+Vector3d(bounds.across().x*((i+1)/(double)width),bounds.across().y*((j+1)/(double)width),bounds.across().z))))->id());
        }
    }
    for (int i=width*width;i<num;++i) {
        typedef boost::uniform_int<> distribution_type;
        typedef boost::variate_generator<base_generator_type&, distribution_type> gen_type;
        gen_type gen(generator, distribution_type(0, servers.size()-1));

        servers.push_back(gSpaceNodes[servers[gen()]]->split()->id());
    }
    for (size_t i=0;i<servers.size();++i) {
        //std::cout<< "Bounds are "<<gSpaceNodes[servers[i]]->bounds().min().toString()<<'-'<<gSpaceNodes[servers[i]]->bounds().min().toString()<<'\n';
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
void generateObjects(int nobj,const std::vector<UUID> &objectHosts) {
    size_t nss=gSpaceNodes.size();
    for(std::tr1::unordered_map<UUID,SpaceNode*,UUID::Hasher>::iterator i=gSpaceNodes.begin(),ie=gSpaceNodes.end();i!=ie;++i) {
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
void generateObjectHosts(int noh, int nobj, bool separateObjectStreams,bool distanceKnowledge,bool remoteRadiusKnowledge,bool localRadiusKnowledge) {
    std::vector<UUID> ohs;
    for (int i=0;i<noh;++i) {
        ohs.push_back((new ObjectHost(separateObjectStreams,distanceKnowledge,remoteRadiusKnowledge,localRadiusKnowledge))->id());
    }
    generateObjects(nobj,ohs);
}
int main() {
    int nobj=8192;
    int nssv=128;
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
    generateSpaceServers(bounds, nssv,toplevelgridwidth);
    generateObjectHosts(noh,nobj,separateObjectStreams,distanceKnowledge,remoteRadiusKnowledge,localRadiusKnowledge);
    Generator *rmg=new RandomMessageGenerator;
    std::map<double,int> finalMessageOrder;
    if (false) {
        OracleOHMessageQueue omq;
        for (int i=0;i<nmsg;++i) {
            omq.insertMessage(rmg->generate(oSeg.random()));
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
            finalMessageOrder[-priority]=i;
            //std::cout<<gPriority(msg.source,msg.dest)<<std::endl;
            ++i;
        }
    }else {
        for (int i=0;i<nmsg;++i) {
            Message msg=rmg->generate(oSeg.random());
            gObjectHosts[oSeg[msg.source]->objectHost]->insertMessage(msg);
        }
        for (SpaceNodeMap::iterator i=gSpaceNodes.begin(),ie=gSpaceNodes.end();i!=ie;++i) {
            i->second->pullFromOH(ohQueueSize);
        }
        for (SpaceNodeMap::iterator i=gSpaceNodes.begin(),ie=gSpaceNodes.end();i!=ie;++i) {
            i->second->pullFromSpaces(spaceQueueSize);
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
                    i->second->pullFromOH(ohQueueSize);
                }
                for (SpaceNodeMap::iterator i=gSpaceNodes.begin(),ie=gSpaceNodes.end();i!=ie;++i) {
                    i->second->pullFromSpaces(spaceQueueSize);
                }
            }
            if (nonepulled&&i<nmsg) {
                for (SpaceNodeMap::iterator i=gSpaceNodes.begin(),ie=gSpaceNodes.end();i!=ie;++i) {
                    std::cerr<<"q "<<i->second->waitingMessagesOH()<<' '<<i->second->waitingMessagesSpace()<<' '<<i->second->waitingMessagesRN()<<'\n';
                    i->second->pullFromSpaces(128);
                }
                std::cerr<<"Fatal error"<<'\n';
                
            }
        }
    }
    int64 totalError=0;
    std::map<double,int>::iterator iter=finalMessageOrder.begin();
    for (size_t i=0;i<finalMessageOrder.size();++i,++iter) {
        int64 diff=i-iter->second;
        if (diff) {
            //std::cout<<"Diffd "<<diff<<"'\n";
        }
        if (diff<0) diff=-diff;
        totalError+=diff;
    }
    std::cout<<"Total error: "<<totalError<<'\n';
    return 0;
}
