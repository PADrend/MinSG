/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT

#include <Util/Utils.h>
#include <Util/Macros.h>
#include <Util/Network/NetworkTCP.h>
#include <Util/Network/NetworkUDP.h>
#include <Util/Concurrency/Concurrency.h>
#include <Util/Concurrency/Mutex.h>
#include <Util/Concurrency/Lock.h>
#include <Util/GenericAttribute.h>
#include <Util/IO/FileName.h>
#include <Util/IO/FileUtils.h>
#include <Util/IO/TemporaryDirectory.h>

#include "Message.h"
#include "Session.h"
#include "TCPHandler.h"
#include "UDPHandler.h"
#include "Tools.h"
#include "MessageDispatcher.h"

#include "ClientUnit.h"

#include <cstdint>
#include <memory>
#include <string>
#include <sstream>
#include <vector>

#define MAX_UDP_PACKET_SIZE 4096

#define EVENT_NEW_SESSION "EVENT_NEW_SESSION"
#define EVENT_CONNECTED "EVENT_CONNECTED"
#define EVENT_DISCONNECTED "EVENT_DISCONNECTED"

using namespace D3Fact;

static const Util::StringIdentifier EVENT_ID("id");
static const Util::StringIdentifier EVENT_SESSION_ID("sessionId");

using namespace Util;
using namespace Util::Network;

const long kEVENT_CONNECTED = Tools::crc32("EVENT_CONNECTED");

ClientUnit::ClientUnit(const std::string & remoteHost_, uint16_t remoteTCP_, uint16_t remoteUDP_, uint8_t maxConnections_, uint32_t msgTimeout_) :
		clientId(0), connected(false), maxConnections(maxConnections_), msgTimeout(msgTimeout_), udpHandler(nullptr) {
	remoteTCP = IPv4Address::resolveHost(remoteHost_, remoteTCP_);
	remoteUDP = IPv4Address::resolveHost(remoteHost_, remoteUDP_);
	sendMutex = Concurrency::createMutex();
	sessionMutex = Concurrency::createMutex();
	eventMutex = Concurrency::createMutex();
	connectMutex = Concurrency::createMutex();
	tempDir = new Util::TemporaryDirectory("D3Fact");
}

ClientUnit::~ClientUnit() {
	close();
	delete sendMutex;
	delete sessionMutex;
	delete udpHandler;
	delete connectMutex;
	delete eventMutex;
}

bool ClientUnit::connect() {
	if(connected || !tcpPool.empty())
		return false;

	for(uint_fast8_t i=0; i<maxConnections; ++i) {
		tcpPool.push_back(new TCPHandler(this));
	}

	for(auto & elem : tcpPool) {
		if(!(elem)->start())
			WARN("Could not start TCPHandler.");
	}

	UDPNetworkSocket* socket = createUDPNetworkSocket(0, MAX_UDP_PACKET_SIZE);
	if(!socket) {
		WARN("UDP Handler could not be initiated.");
	} else {
		socket->addTarget(remoteUDP);
		udpHandler = new UDPHandler(socket, this);
		if(!udpHandler->start()) {
			WARN("UDP Handler could not be started.");
		}
	}

	return true;

}

int32_t ClientUnit::startSession() {
	if(!connected) {
		WARN("Client not connected.");
		return 0;
	}
	int32_t sessionId = 0;
	auto sessionLock = Concurrency::createLock(*sessionMutex);

	while(sessionId == 0 || sessionMap.count(sessionId) > 0 || openSessionRequests.count(sessionId) > 0 ) {
		sessionId = Tools::generateId();
	}
	openSessionRequests.insert(sessionId);

	// request session from server
	std::vector<uint8_t> data(sizeof(sessionId));
	Tools::putIntBig(data, 0, sessionId);
	asyncSend(Message::get(0, 0, MSGTYPE_START_NEW_SESSION, data));

	return sessionId;
}

Session * ClientUnit::getSession(int32_t sessionId_) {
	auto sessionLock = Concurrency::createLock(*sessionMutex);
	if(sessionMap.count(sessionId_)>0) {
		return sessionMap[sessionId_].get();
	} else {
		return nullptr;
	}
}

void ClientUnit::closeSession(Session * session_) {
	std::cout << "closing session " << session_->getSessionId() << "...";
	auto sessionLock = Concurrency::createLock(*sessionMutex);
	if(session_ && sessionMap.count(session_->getSessionId())>0) {
		std::vector<uint8_t> body(sizeof(int32_t));
		Tools::putIntBig(body, 0, session_->getSessionId());
		asyncSend(Message::get(clientId,0,MSGTYPE_CLOSE_SESSION,session_->getSessionId(), body));
		sessionMap.erase(session_->getSessionId());
	}
	std::cout << "done" << std::endl;
}


void ClientUnit::dispatchMessages(uint32_t maxWorkload/*=0xffffffff*/, bool async/*=false*/) {
	std::deque<Util::Reference<Session> > sessions;
	{
		auto sessionLock = Concurrency::createLock(*sessionMutex);
		for(auto session : sessionMap) {
			sessions.push_back(session.second);
		}
	}
	for(auto session : sessions) {
		session->getDispatcher()->dispatch(maxWorkload, async);
	}
}

void ClientUnit::distributeReceivedMSG(Message * msg_) {
	int32_t sessionId = msg_->getSession();
	if(sessionId == 0) {
		handleSession0Messages(msg_);
	} else {
		auto sessionLock = Concurrency::createLock(*sessionMutex);
		if(sessionMap.count(sessionId) > 0) {
			Reference<Session> session = sessionMap[sessionId];
			session->received(msg_);
		} else {
			/*std::stringstream ss;
			ss << "Message (" << msg_->getType() << ") contained wrong session ID (" << sessionId << ") or session already closed.";
			WARN(ss.str());*/
			msg_->dispose();
		}
	}
}

void ClientUnit::handleSession0Messages(Message * msg_) {
	FAIL_IF(msg_->getSession() != 0);
	auto sessionLock = Concurrency::createLock(*sessionMutex);
	const std::vector<uint8_t> & body = msg_->getBody();
	int32_t sessionId = Tools::getInt(body, 0);
	Session* session = nullptr;
	std::cout << "Reveived session 0 message " << sessionId << " " << msg_->getType() << "...";
	switch(msg_->getType()) {
	case MSGTYPE_START_NEW_SESSION:
		if(sessionId != 0 && (sessionMap.count(sessionId) == 0 || sessionMap.at(sessionId).isNotNull()) ) {
			sessionMap[sessionId] = Reference<Session>(new Session(sessionId, this));
			asyncSend(Message::get(0,0,MSGTYPE_ACK_NEW_SESSION, body));
		} else {
			asyncSend(Message::get(0,0,MSGTYPE_ERROR_NEW_SESSION, body));
		}
		break;
	case MSGTYPE_ACK_NEW_SESSION:
		openSessionRequests.erase(sessionId);
		sessionMap[sessionId] = Reference<Session>(new Session(sessionId, this));

		this->postEvent(createEvent(EVENT_NEW_SESSION, sessionId));
		break;
	case MSGTYPE_ERROR_NEW_SESSION:
		{
			std::stringstream ss;
			ss << "Server could not create session '" << sessionId << "'.";
			WARN(ss.str());
		}
		openSessionRequests.erase(sessionId);
		sessionMap.erase(sessionId);
		break;
	case MSGTYPE_CLOSE_SESSION:
		/*session = sessionMap[sessionId].get();
		if(session)
			session->close();
		openSessionRequests.erase(sessionId);
		sessionMap.erase(sessionId);
		asyncSend(Message::get(0,0,MSGTYPE_ERROR_NEW_SESSION, body));*/
		break;
	case MSGTYPE_ACK_CLOSE_SESSION:
		break;
	default:
		WARN("Message with session id 0 received.");
		break;
	}
	std::cout << "done" << std::endl;
	msg_->dispose();
}

void ClientUnit::asyncSend(Message * msg_) {
	auto lock = Concurrency::createLock(*sendMutex);
	if(msg_->getProtocol() == Message::TCP) {
		sendTCP.push_back(msg_);
	} else if(msg_->getProtocol() == Message::UDP) {
		udpHandler->sendMSG(msg_);
	}
}

Message* ClientUnit::getNextTCPMSG() {
	auto lock = Concurrency::createLock(*sendMutex);
	if(sendTCP.empty())
		return nullptr;
	Message* msg = sendTCP.front();
	sendTCP.pop_front();
	return msg;
}

void ClientUnit::close() {
	{
		auto lock = Concurrency::createLock(*sendMutex);
		sendTCP.clear();
	}
	postEvent(createEvent(EVENT_DISCONNECTED, 0));
	// FIXME: Might result in deadlock
	//auto sessionLock = Concurrency::createLock(*sessionMutex);
	openSessionRequests.clear();
	for(auto ses : sessionMap) {
		ses.second->close();
	}
	sessionMap.clear();
	for(auto & it : tcpPool) {
		it->close();
		delete it;
	}
	tcpPool.clear();
	udpHandler->close();
	connected = false;
	ClientUnit::clients.erase(this->clientId);
}

bool ClientUnit::isConnected() {

	if(connected) {
		bool stillAlive = false;
		for(auto & elem : tcpPool) {
			if((elem)->isConnected()) {
				stillAlive = true;
				break;
			}
		}
		if(!stillAlive) {
			close();
		}
	}
	return connected;
}

const Util::FileName & ClientUnit::getTempPath() const {
	return tempDir->getPath();
}

Util::GenericAttribute* ClientUnit::pollEvent() {
	auto lock = Concurrency::createLock(*eventMutex);
	if(eventQueue.empty())
		return nullptr;
	Util::GenericAttribute* event = eventQueue.front();
	eventQueue.pop_front();
	return event;
}

void ClientUnit::postEvent(Util::GenericAttribute* event) {
	auto lock = Concurrency::createLock(*eventMutex);
	eventQueue.push_back(event);
}

Util::GenericAttributeMap* ClientUnit::createEvent(const std::string& eventId, int32_t sessionId/*=0*/) {
	auto event = new Util::GenericAttributeMap();
	event->setString(EVENT_ID, eventId);
	event->setValue(EVENT_SESSION_ID, Util::GenericAttribute::createNumber(sessionId));
	return event;
}

bool ClientUnit::setClientId(int32_t id) {
	auto lock = Concurrency::createLock(*sendMutex);
	if(clientId == 0) {
		clientId = id;
		connected = true;
		postEvent(createEvent(EVENT_CONNECTED, 0));
		ClientUnit::clients[id] = this;
		return true;
	} else {
		return clientId == id;
	}
}

std::map<int32_t, Util::Reference<ClientUnit> > ClientUnit::clients;

ClientUnit* ClientUnit::getClient(int32_t id) {
	auto it = clients.find(id);
	if(it != clients.end())
		return it->second.get();
	return nullptr;
}

#endif /* MINSG_EXT_D3FACT */
