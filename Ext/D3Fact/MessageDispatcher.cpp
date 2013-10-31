/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT

#include <string>
#include <sstream>

#include <Util/Macros.h>
#include <Util/Concurrency/Concurrency.h>
#include <Util/Concurrency/Mutex.h>
#include <Util/Concurrency/Lock.h>

#include "Message.h"
#include "Session.h"
#include "MessageHandler.h"
#include "MessageDispatcher.h"

#include <deque>

using namespace D3Fact;
using namespace Util;

MessageDispatcher::MessageDispatcher(Session* session_) : session(session_),
		mutex(Util::Concurrency::createMutex()), sessionMutex(Util::Concurrency::createMutex()) {
}

MessageDispatcher::~MessageDispatcher() {
	delete mutex;
	delete sessionMutex;
}

void MessageDispatcher::dispatch(uint32_t maxWorkload/*=0xffffffff*/, bool async/*=false*/) {

	{
		auto lockSession = Util::Concurrency::createLock(*sessionMutex);
		if(session.isNull())
			return;

		for(uint32_t i=0; i<maxWorkload && session->hasReceivedMSG(); ++i) {
			Message* msg = session->receiveMSG();
			if(msg) {
				auto lock = Util::Concurrency::createLock(*mutex);
				if(handler.count(0)>0) {
					handler[0]->addMessage(msg);
				}
				if(handler.count(msg->getType())>0) {
					handler[msg->getType()]->addMessage(msg);
				} else if(handler.count(0)==0) {
					std::stringstream ss;
					ss << "There is no handler for messages of type '" << msg->getType() << "' in session '" << session->getSessionId() << "'.";
					WARN(ss.str());
					msg->dispose();
				}
			}
		}
	}

	if(async)
		return;

	std::deque<Util::Reference<MessageHandler> > tmp;
	{
		auto lock = Util::Concurrency::createLock(*mutex);
		for(auto it : handler) {
			if(it.second->getMode() == MessageHandler::SYNC) {
				tmp.push_back(it.second);
			}
		}
	}
	for(auto handler_ : tmp) {
		handler_->handleMessages();
	}
}

void MessageDispatcher::registerHandler(MessageHandler* msgHandler, int32_t type) {
	auto lockSession = Util::Concurrency::createLock(*sessionMutex);
	auto lock = Util::Concurrency::createLock(*mutex);
	if(handler.count(type)>0) {
		std::stringstream ss;
		ss << "Message handler for type '" << type << "' is already registered for session '" << session->getSessionId() << "'.";
		WARN(ss.str());
		return;
	}
	handler[type] = msgHandler;
}

void MessageDispatcher::unregisterHandler(int32_t type) {
	auto lock = Util::Concurrency::createLock(*mutex);
	handler.erase(type);
}

MessageHandler* D3Fact::MessageDispatcher::getHandler(int32_t type) {
	auto lock = Util::Concurrency::createLock(*mutex);
	if(handler.count(type) == 0)
		return nullptr;
	return handler[type].get();
}

bool MessageDispatcher::hasHandler(int32_t type) {
	auto lock = Util::Concurrency::createLock(*mutex);
	return handler.count(type) > 0;
}

void MessageDispatcher::dispose() {
	auto lockSession = Util::Concurrency::createLock(*sessionMutex);
	auto lock = Util::Concurrency::createLock(*mutex);
	handler.clear();
	session.detachAndDecrease();
}

#endif /* MINSG_EXT_D3FACT */
