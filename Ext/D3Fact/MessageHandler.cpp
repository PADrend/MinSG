/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT

#include "Message.h"
#include "MessageHandler.h"

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

using namespace D3Fact;
using namespace Util;

namespace D3Fact {
class WorkerThread {
public:
	WorkerThread(MessageHandler* handler_);
	~WorkerThread();
	void wake();
	void close();
protected:
	void run();
	bool closed;
	std::mutex wakeMutex;
	std::condition_variable wakeCV;
	MessageHandler* handler;
	std::thread thread;
};
}

WorkerThread::WorkerThread(MessageHandler* handler_) : closed(false), wakeMutex(), wakeCV(), handler(handler_),
	thread(std::bind(&WorkerThread::run, this)) {
}

WorkerThread::~WorkerThread() {
	close();
	thread.join();
}

void WorkerThread::wake() {
	wakeCV.notify_all();
}

void WorkerThread::close() {
	closed = true;
	wakeCV.notify_all();
}

void WorkerThread::run() {
	while(!closed){
		std::unique_lock<std::mutex> lock(wakeMutex);
		wakeCV.wait(lock);
		handler->handleMessages();
	}
}

// -----------------------------------------------------

MessageHandler::MessageHandler(Mode_t mode_) : ReferenceCounter_t(), Util::AttributeProvider(), mode(mode_), queueMutex() {
	worker = mode == ASYNC ? new WorkerThread(this) : nullptr;
}

MessageHandler::~MessageHandler() {
	delete worker;
}

void MessageHandler::addMessage(Message* msg) {
	{
		std::lock_guard<std::mutex> lock(queueMutex);
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
	std::lock_guard<std::mutex> lock(queueMutex);
	Message* msg = nullptr;
	if(!msgQueue.empty()) {
		msg = msgQueue.front();
		msgQueue.pop_front();
	}
	return msg;
}


#endif /* MINSG_EXT_D3FACT */
