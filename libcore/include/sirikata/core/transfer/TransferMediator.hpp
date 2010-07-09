/*  Sirikata Transfer -- Content Transfer mediation
 *  TransferMediator.hpp
 *
 *  Copyright (c) 2010, Jeff Terrace
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
/*  Created on: Jan 15, 2010 */

#ifndef SIRIKATA_TransferMediator_HPP__
#define SIRIKATA_TransferMediator_HPP__

#include "SimplePriorityAggregation.hpp"

#include <iostream>
#include <map>
#include <vector>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/format.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/if.hpp>
#include <boost/lambda/loops.hpp>
#include <boost/lambda/switch.hpp>
#include <boost/lambda/construct.hpp>
#include <boost/lambda/casts.hpp>
#include <boost/lambda/exceptions.hpp>
#include <boost/lambda/algorithm.hpp>
#include <boost/lambda/numeric.hpp>
#include <boost/asio.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/Asio.hpp>
#include <sirikata/core/util/Thread.hpp>
#include <sirikata/core/util/Singleton.hpp>
#include <sirikata/core/task/EventManager.hpp>

namespace Sirikata {
namespace Transfer {

using namespace boost::multi_index;

/*
 * Mediates requests for files
 */
class SIRIKATA_EXPORT TransferMediator : public AutoSingleton<TransferMediator>{

	class AggregateRequest {
	public:
		TransferRequest::PriorityType mPriority;

	private:
		std::map<std::string, std::tr1::shared_ptr<TransferRequest> > mTransferReqs;
		const std::string mIdentifier;

		void updateAggregatePriority() {
			TransferRequest::PriorityType newPriority = SimplePriorityAggregation::aggregate(mTransferReqs);
			mPriority = newPriority;
		}

	public:

		const std::map<std::string, std::tr1::shared_ptr<TransferRequest> > & getTransferRequests() const {
			return mTransferReqs;
		}

		std::tr1::shared_ptr<TransferRequest> getSingleRequest() {
		    std::map<std::string, std::tr1::shared_ptr<TransferRequest> >::iterator it = mTransferReqs.begin();
		    return it->second;
		}

		void setClientPriority(std::tr1::shared_ptr<TransferRequest> req) {
		    const std::string& clientID = req->getClientID();http://www.facebook.com/
		    std::map<std::string, std::tr1::shared_ptr<TransferRequest> >::iterator findClient = mTransferReqs.find(clientID);
			if(findClient == mTransferReqs.end()) {
			    mTransferReqs[clientID] = req;
			    updateAggregatePriority();
			} else if(findClient->second->getPriority() != req->getPriority()) {
				findClient->second = req;
				updateAggregatePriority();
			} else {
			    findClient->second = req;
			}
		}

		const std::string & getIdentifier() const {
			return mIdentifier;
		}

		TransferRequest::PriorityType getPriority() const {
			return mPriority;
		}

		AggregateRequest(std::tr1::shared_ptr<TransferRequest> req)
			: mIdentifier(req->getIdentifier()) {
			setClientPriority(req);
		}
	};

	boost::mutex mAggMutex; //lock this to access mAggregatedList

	//tags
	struct tagID{};
	struct tagPriority{};

	typedef multi_index_container<
		std::tr1::shared_ptr<AggregateRequest>,
		indexed_by<
			hashed_unique<tag<tagID>, const_mem_fun<AggregateRequest,const std::string &,&AggregateRequest::getIdentifier> >,
			ordered_non_unique<tag<tagPriority>, member<AggregateRequest,TransferRequest::PriorityType,&AggregateRequest::mPriority> >
		>
	> AggregateList;
	AggregateList mAggregateList;

	//access iterators
	typedef AggregateList::index<tagID>::type AggregateListByID;
	typedef AggregateList::index<tagPriority>::type AggregateListByPriority;

	class PoolWorker {
		std::tr1::shared_ptr<TransferPool> mTransferPool;
		Thread * mWorkerThread;
		bool mCleanup;
		TransferMediator * mParent;

	public:
		PoolWorker(std::tr1::shared_ptr<TransferPool> transferPool, TransferMediator * parent)
			: mTransferPool(transferPool), mCleanup(false), mParent(parent) {
			mWorkerThread = new Thread(std::tr1::bind(&PoolWorker::run, this));
		}

		PoolWorker(const PoolWorker & other)
			: mTransferPool(other.mTransferPool), mWorkerThread(other.mWorkerThread), mCleanup(other.mCleanup), mParent(other.mParent) {
		}

		std::tr1::shared_ptr<TransferPool> getTransferPool() const {
			return mTransferPool;
		}

		Thread * getThread() const {
			return mWorkerThread;
		}

		void cleanup() {
			mCleanup = true;
		}

		void run() {
			while(!mCleanup) {
				std::tr1::shared_ptr<TransferRequest> req = mTransferPool->getRequest();
				if(req == NULL) {
					continue;
				}
				//SILOG(transfer, debug, "worker got one!");

				boost::unique_lock<boost::mutex> lock(mParent->mAggMutex);
				AggregateListByID::iterator findID = mParent->mAggregateList.get<tagID>().find(req->getIdentifier());

				//Check if this request already exists
				if(findID != mParent->mAggregateList.end()) {
					//store original aggregated priority for later
					TransferRequest::PriorityType oldAggPriority = (*findID)->getPriority();

					//Update the priority of this client
					(*findID)->setClientPriority(req);

					//And check if it's changed, we need to update the index
					TransferRequest::PriorityType newAggPriority = (*findID)->getPriority();
					if(oldAggPriority != newAggPriority) {
						using boost::lambda::_1;
						//Convert the iterator to the priority one and update
						AggregateListByPriority::iterator byPriority = mParent->mAggregateList.project<tagPriority>(findID);
						AggregateListByPriority & priorityIndex = mParent->mAggregateList.get<tagPriority>();
						priorityIndex.modify_key(byPriority, _1=newAggPriority);
					}
				} else {
					//Make a new one and insert it
					//SILOG(transfer, debug, "worker id " << mTransferPool->getClientID() << " adding url " << req->getIdentifier());
					std::tr1::shared_ptr<AggregateRequest> newAggReq(new AggregateRequest(req));
					mParent->mAggregateList.insert(newAggReq);
				}

			}
		}
	};

	Task::GenEventManager* mEventSystem;
	Network::IOService* mIOService;

	typedef std::map<std::string, std::tr1::shared_ptr<PoolWorker> > PoolType;
	PoolType mPools;
	boost::shared_mutex mPoolMutex; //lock this to access mPools

	bool mCleanup;
	uint32 mNumOutstanding;

public:
	static TransferMediator& getSingleton();
	static void destroy();

	Thread *thread;
    
	/*
	 * Initializes the transfer mediator with the components it needs to fulfill requests
	 */
	void initialize(Task::GenEventManager* eventSystem, Network::IOService* io) {
	    mEventSystem = eventSystem;
	    mIOService = io;
	    mCleanup = false;
	    mNumOutstanding = 0;
	    thread = new Thread(std::tr1::bind(&TransferMediator::mediatorThread, this));
	}

	/*
	 * Used to register a client that has a pool of requests it needs serviced by the transfer mediator
	 * @param clientID	Should be a string that uniquely identifies the client
	 * @param listener	An EventListener to receive a TransferEventPtr with the retrieved data.
	 */
	std::tr1::shared_ptr<TransferPool> registerClient(const std::string clientID) {
		std::tr1::shared_ptr<TransferPool> ret(new TransferPool(clientID));

		//Lock exclusive to access map
		boost::upgrade_lock<boost::shared_mutex> lock(mPoolMutex);
		boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);

		//ensure client id doesnt already exist, they should be unique
		PoolType::iterator findClientId = mPools.find(clientID);
		assert(findClientId == mPools.end());

		std::tr1::shared_ptr<PoolWorker> worker(new PoolWorker(ret, this));
		mPools.insert(PoolType::value_type(clientID, worker));

		return ret;
	}

	/*
	 * Call when system should be shut down
	 */
	void cleanup() {
		mCleanup = true;
	}

	void execute_finished(std::tr1::shared_ptr<TransferRequest> req, std::string id) {
        boost::unique_lock<boost::mutex> lock(mAggMutex, boost::defer_lock_t());
        lock.lock();

        AggregateListByID& idIndex = mAggregateList.get<tagID>();
        AggregateListByID::iterator findID = idIndex.find(id);
        if(findID == idIndex.end()) {
            SILOG(transfer, error, "Got a callback in TransferMediator from a TransferRequest with no associated ID");
            mNumOutstanding--;
            lock.unlock();
            return;
        }

        const std::map<std::string, std::tr1::shared_ptr<TransferRequest> >&
            allReqs = (*findID)->getTransferRequests();

        for(std::map<std::string, std::tr1::shared_ptr<TransferRequest> >::const_iterator
                it = allReqs.begin(); it != allReqs.end(); it++) {
            SILOG(transfer, debug, "Notifying a caller that TransferRequest is complete");
            it->second->notifyCaller(req);
        }

        mAggregateList.erase(findID);

        mNumOutstanding--;
        lock.unlock();
        SILOG(transfer, debug, "done transfer mediator execute_finished");
        checkQueue();
	}

	void checkQueue() {
	    boost::unique_lock<boost::mutex> lock(mAggMutex, boost::defer_lock_t());

        lock.lock();

        AggregateListByPriority & priorityIndex = mAggregateList.get<tagPriority>();
        AggregateListByPriority::iterator findTop = priorityIndex.begin();

        if(findTop != priorityIndex.end()) {
            std::string topId = (*findTop)->getIdentifier();

            SILOG(transfer, debug, priorityIndex.size() << " length agg list, top priority "
                    << (*findTop)->getPriority() << " id " << topId);

            std::tr1::shared_ptr<TransferRequest> req = (*findTop)->getSingleRequest();

            if(mNumOutstanding == 0) {
                mNumOutstanding++;
                req->execute(req, std::tr1::bind(&TransferMediator::execute_finished, this, req, topId));
            }

        } else {
            //SILOG(transfer, debug, priorityIndex.size() << " length agg list");
        }

        lock.unlock();
	}

	/*
	 * Main thread that handles the input pools
	 */
	void mediatorThread() {
		while(!mCleanup) {
		    checkQueue();
			boost::this_thread::sleep(boost::posix_time::milliseconds(500));
		}
		for(PoolType::iterator pool = mPools.begin(); pool != mPools.end(); pool++) {
			pool->second->cleanup();
			pool->second->getTransferPool()->addRequest(std::tr1::shared_ptr<TransferRequest>());
		}
		for(PoolType::iterator pool = mPools.begin(); pool != mPools.end(); pool++) {
			pool->second->getThread()->join();
		}
	}

};

}
}

#endif /* SIRIKATA_TransferMediator_HPP__ */
