/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT

#include <Util/Concurrency/UserThread.h>
#include <Util/Concurrency/Concurrency.h>
#include <Util/Concurrency/Mutex.h>
#include <Util/Concurrency/Semaphore.h>
#include <Util/Concurrency/Lock.h>

#include "Message.h"
#include "MessageHandler.h"

#include <iostream>

using namespace D3Fact;
using namespace Util;
using namespace Util::Concurrency;

namespace D3Fact {
class WorkerThread : public UserThread {
public:
	WorkerThread(MessageHandler* handler_);
	virtual ~WorkerThread();
	void wake();
	void close();
protected:
	void run() override;
	bool closed;
	Semaphore* wakeMutex;
	MessageHandler* handler;
};
}

WorkerThread::WorkerThread(MessageHandler* handler_) : UserThread(), closed(false), handler(handler_) {
	wakeMutex = Concurrency::createSemaphore();
	start();
}

WorkerThread::~WorkerThread() {
	close();
	join();
	delete wakeMutex;
}

void WorkerThread::wake() {
	wakeMutex->post();
}

void WorkerThread::close() {
	closed = true;
	wakeMutex->post();
}

void WorkerThread::run() {
	while(!closed){
		wakeMutex->wait();
		handler->handleMessages();
	}
}

// -----------------------------------------------------

MessageHandler::MessageHandler(Mode_t mode_) : ReferenceCounter_t(), Util::AttributeProvider(), mode(mode_) {
	queueMutex = Concurrency::createMutex();
	worker = mode == ASYNC ? new WorkerThread(this) : nullptr;
}

MessageHandler::~MessageHandler() {
	delete worker;
	delete queueMutex;
}

void MessageHandler::addMessage(Message* msg) {
	{
		auto lock = Util::Concurrency::createLock(*queueMutex);
		msgQueue.push_back(msg);
	}
	if(mode == ASYNC)
		worker->wake();
}

void MessageHandler::handleMessages() {
	Message* msg = nullptr;
	do {
		msg = nextMessage();
		if(msg)
			handleMessage(msg);
	} while(msg);
}

Message* MessageHandler::nextMessage() {
	auto lock = Util::Concurrency::createLock(*queueMutex);
	Message* msg = nullptr;
	if(!msgQueue.empty()) {
		msg = msgQueue.front();
		msgQueue.pop_front();
	}
	return msg;
}


#endif /* MINSG_EXT_D3FACT */
