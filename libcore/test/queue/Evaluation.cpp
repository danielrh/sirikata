#include "util/Platform.hpp"
#include "util/UUID.hpp"
#include "Message.hpp"
#include "Priority.hpp"
#include "Evaluation.hpp"

namespace Sirikata { namespace QueueBench {

static double getPriority(const Message&msg) {
    return gPriority(msg.source,
                     msg.dest);
}
void evaluateError(const std::vector<Message>&messages,
                   const MessagePriorityMap&finalMessageOrder,
                   const MessagePriorityMap&oracleMessageOrder,
                   int numBins,
                   bool binByPriority) { 
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
    MessagePriority priorityComp(standardfalloff);
    std::vector<Message> inputMessages=messages;
    std::sort(inputMessages.begin(),inputMessages.end(),priorityComp);
    double most=1.0;
    double least=0.0;
    if (messages.size()) {

        most=getPriority(inputMessages.front());
        least=getPriority(inputMessages.back());
    }
    std::vector<Message>::iterator inputiter=inputMessages.begin();
    std::vector<Message>::iterator inputiterend=inputMessages.end();
    MessagePriorityMap::const_iterator outputiter=finalMessageOrder.begin();
    MessagePriorityMap::const_iterator outputiterend=finalMessageOrder.end();
    int totaloffset=0;
    int totalOutputMessageCount=0;
    for (int i=0;i<numBins;++i) {
        Message minpriority;
        int binMessageCount=0;
        for(;inputiter!=inputiterend;++inputiter) {
            
            if(binByPriority==false&&totaloffset++>((double)i)*inputMessages.size()/(double)numBins) {
                break;
            }
            double priority=getPriority(*inputiter);
            if(binByPriority&&(priority-least)/(most-least)>(numBins-1-(double)i)/(double)numBins) {
                break;
            }
            ++binMessageCount;
            minpriority=*inputiter;
        }
        int j;
        for (j=0;j<binMessageCount&&outputiter!=outputiterend;++outputiter,++j) {
            if (priorityComp(minpriority,outputiter->first))
                break;
            ++totalOutputMessageCount;
        }
        std::cout<<'@'<<getPriority(minpriority)<<':'<<j<<'/'<<binMessageCount<<'\n';
        
        
    }
    std::cout<<totalOutputMessageCount<<'/'<<inputMessages.size()<<'\n';
}
} }
