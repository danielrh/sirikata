#include "util/Platform.hpp"
#include "util/UUID.hpp"
#include "Message.hpp"
#include "Priority.hpp"
#include "Evaluation.hpp"
#include "OSeg.hpp"
namespace Sirikata { namespace QueueBench {

static double getPriority(const Message&msg) {
    return standardfalloff(msg.source,
                     msg.dest);
}



void evaluateRadiusBasedError(std::vector<Message> &inputMessages,
                              const std::vector<Message>&finalMessages,
                              int numBins, 
                              bool binByPriority) {

    MessagePriority priorityComp(standardfalloff);
    std::sort(inputMessages.begin(),inputMessages.end(),priorityComp);
    double most=1.0;
    double least=0.0;
    if (inputMessages.size()) {

        most=getPriority(inputMessages.front());
        least=getPriority(inputMessages.back());
    }
    std::vector<Message>::iterator inputiter=inputMessages.begin();
    std::vector<Message>::iterator inputiterend=inputMessages.end();
    std::vector<Message>::const_iterator outputiter=finalMessages.begin();
    std::vector<Message>::const_iterator outputiterend=finalMessages.end();
    int totaloffset=0;
    int totalOutputMessageCount=0;
    for (int i=0;i<numBins;++i) {
        Message minpriority;
        int binMessageCount=0;
        for(;inputiter!=inputiterend;++inputiter) {
            
            if(binByPriority==false&&totaloffset++>((double)i+1)*inputMessages.size()/(double)numBins) {
                break;
            }
            double priority=getPriority(*inputiter);

            if(binByPriority&&(priority-least)/(most-least)<(numBins-1-(double)i)/(double)numBins) {
                //std::cout << "Pri "<<priority<<"least "<<least<<" most "<<most<< " vs "<<(numBins-1-(double)i)/(double)numBins<<'\n';
                
                break;
            }
            ++binMessageCount;
            minpriority=*inputiter;
        }
        int j;
        for (j=0;j<binMessageCount&&outputiter!=outputiterend;++outputiter,++j) {
            if (priorityComp(minpriority,*outputiter))
                break;
            ++totalOutputMessageCount;
        }
        if (binMessageCount)
            std::cout<<' '<<getPriority(minpriority)<<','<<j<<','<<binMessageCount<<'\n';
        
        
    }
    std::cerr<<totalOutputMessageCount<<'/'<<inputMessages.size()<<'\n';
}

#define NUM_RADIUS_BINS 4
int getRadiusBin(const Message&m, double minRadius, double maxRadius){
    double a=oSeg[m.source]->radialSize;
    double b=oSeg[m.dest]->radialSize;
    if (b*1.5<a) {
        return 1;//big object -> small object
    }
    if (a*1.5<b) {
        return 2;//small object -> big object
    }
    assert(minRadius==a||a-minRadius>=0);
    assert(maxRadius==a||maxRadius-a>=0);
    if (a-minRadius<maxRadius-a) {
        return 0;
    }
    return 3;
}
void evaluateError(const std::vector<Message>&messages,
                   const MessagePriorityMap&finalMessageOrder,
                   const MessagePriorityMap&oracleMessageOrder,
                   int numBins,
                   bool binByPriority) { 
    if (0){//old error mechanism compared total ordering of messages
        int64 totalError=0;
        int64 maxError=0;
        std::map<Message,int,MessagePriority>::const_iterator iter=finalMessageOrder.begin();
        for (size_t i=0;i<finalMessageOrder.size();++i,++iter) {
            int64 diff=i-iter->second;
            if (oracleMessageOrder.find(iter->first)!=oracleMessageOrder.end()) {
                diff=oracleMessageOrder.find(iter->first)->second-iter->second;
            }else {
                maxError+=1;
                //std::cout<<"Oracle failed to deliver message "<<iter->first<<"'\n";
        }
            if (diff) {
                //std::cout<<"Diffd "<<diff<<"'\n";
            }
            if (diff<0) diff=-diff;
            totalError+=diff;
        }
        int64 fullError=totalError+finalMessageOrder.size()*maxError;
        {
            std::cout<<"Error: "<<totalError<<'\n';
            std::cout<<"Num undelivered: "<<maxError<<'/'<<oracleMessageOrder.size()<<'\n';
            std::cout<<"Total: "<<fullError<<'\n';
        }
    }
    double maxRadius=0;
    double minRadius=1.e38;
    for (std::vector<UUID>::iterator i=oSeg.mUUIDs.begin(),ie=oSeg.mUUIDs.end();i!=ie;++i) {
        double rad=oSeg[*i]->radialSize;
        if (rad>maxRadius||i==oSeg.mUUIDs.begin()) maxRadius=rad;
        if (rad<minRadius||i==oSeg.mUUIDs.begin()) minRadius=rad;
    }

    std::vector<Message>unsortedMessages[NUM_RADIUS_BINS];
    std::vector<Message>finalMessages[NUM_RADIUS_BINS];
    
    for (std::vector<Message>::const_iterator i=messages.begin(),ie=messages.end();i!=ie;++i) {
        unsortedMessages[getRadiusBin(*i,minRadius,maxRadius)].push_back(*i);
    }
    for (MessagePriorityMap::const_iterator i=finalMessageOrder.begin(),ie=finalMessageOrder.end();i!=ie;++i) {
        finalMessages[getRadiusBin(i->first,minRadius,maxRadius)].push_back(i->first);
    }
    for (int r=0;r<NUM_RADIUS_BINS;++r) {
        switch(r){
          case 0:std::cout<<"Similar Size Small Objects\n";
            break;
          case 1:std::cout<<"Large Sender\n";
            break;
          case 2:std::cout<<"LargeReceiver\n";
            break;
          case 3:std::cout<<"Similar Size Large Objects\n";
            break;            
        }
        evaluateRadiusBasedError(unsortedMessages[r],finalMessages[r], numBins,  binByPriority);
    }
}
} }
