/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT

#include <limits>

#include <Util/Concurrency/Concurrency.h>
#include <Util/Concurrency/Mutex.h>
#include <Util/Concurrency/Lock.h>
#include <Util/Macros.h>

#include "MessageDispatcher.h"
#include "ClientUnit.h"
#include "Message.h"
#include "Session.h"

#include <cstdint>
#include <iostream>
#include <vector>

using namespace D3Fact;
using namespace Util;

Session::Session(int32_t sessionId_, ClientUnit* client_) :
		sessionId(sessionId_), closed(false), client(client_),
		receivedMSGID(std::numeric_limits<int64_t>::min()), sendMSGID(std::numeric_limits<int64_t>::min()), dispatcher(new MessageDispatcher(this)) {
	receivedMutex = Concurrency::createMutex();
}

Session::~Session() {
	close();
}

Message* Session::createMessage(int32_t type, size_t /*bodySize*//*=0*/, int protocol/*=0*/) {
	const std::vector<uint8_t> body;
	if(client.isNull())
		return nullptr;
	return protocol > 0 ? Message::get(client->getClientId(), sessionId, 0, type, body) : Message::get(sessionId, 0, type, body);
}

bool Session::hasReceivedMSG() {
	auto receivedLock = Concurrency::createLock(*receivedMutex);
	return !receivedQueue.empty() && receivedQueue.top()->getOrder() <= receivedMSGID+1;
}

Message* Session::receiveMSG() {
	auto receivedLock = Concurrency::createLock(*receivedMutex);
	if(receivedQueue.empty() || receivedQueue.top()->getOrder() > receivedMSGID+1)
		return nullptr;
	Message* msg = receivedQueue.top();
	if(!msg)
		return nullptr;
	receivedMSGID = msg->getOrder();
	receivedQueue.pop();
	return msg;
}

void Session::received(Message* msg_) {
	auto receivedLock = Concurrency::createLock(*receivedMutex);
	if(msg_->getType() > 0 && (msg_->getProtocol() == Message::TCP || msg_->getOrder() >= receivedMSGID)) {
		receivedQueue.push(msg_);
	} else {
		msg_->dispose();
	}
}

void Session::send(Message* msg) {
	if(msg->getSession() != sessionId) {
		WARN("Wrong session id!");
		return;
	}
	msg->setOrder( msg->getProtocol() == Message::UDP ? sendMSGID : ++sendMSGID);
	if(client.isNotNull())
		client->asyncSend(msg);
}

void Session::close() {
	if(closed)
		return;
	{
		auto receivedLock = Concurrency::createLock(*receivedMutex);
		if(client.isNotNull())
			client->closeSession(this);
		closed = true;
	}
	this->dispatcher->dispose();
}

#endif /* MINSG_EXT_D3FACT */
