/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT

#include <Util/Macros.h>
#include <Util/Utils.h>
#include <Util/Network/NetworkTCP.h>
#include <Util/Concurrency/Concurrency.h>
#include <Util/Concurrency/Lock.h>
#include <Util/Concurrency/Mutex.h>
#include <Util/Concurrency/Thread.h>
#include <Util/GenericAttribute.h>

#include "ClientUnit.h"
#include "Message.h"
#include "Tools.h"
#include "TCPHandler.h"

#include <cstdint>
#include <vector>

#define PROTOCOL_VERSION 3
#define CLIENT_TYPE 2
#define ACK_TYPE_ERR 2

#define EVENT_CONNECTION_FAILED "EVENT_CONNECTION_FAILED"
#define EVENT_TCP_CONNECTED "EVENT_TCP_CONNECTED"
#define EVENT_TCP_DISCONNECTED "EVENT_TCP_DISCONNECTED"

using namespace D3Fact;
using namespace Util;
using namespace Util::Network;

TCPHandler::TCPHandler(ClientUnit* client_) : Concurrency::UserThread(), client(client_), connected(false) {
}

TCPHandler::~TCPHandler() {
	close();
}

void TCPHandler::close() {
	connected = false;
	if(isActive())
		join();
}


void TCPHandler::run() {
	Util::Reference<TCPConnection> connection = TCPConnection::connect(client->remoteTCP);
	if(connection.isNull()) {
		client->postEvent(client->createEvent(EVENT_CONNECTION_FAILED,0));
		return;
	}

	{
		// There should always be only one connection at a time waiting for an init-ack-message
		auto lock = Concurrency::createLock(*client->connectMutex);

		// send init message
		// | length (4 Bytes) | protocol version (1 Byte) | client id (4 Bytes) | udp port (4 Bytes) | client type (1 Byte)
		std::vector<uint8_t> initMsg( 4 + INIT_MSGBODY_LENGTH );
		Tools::putIntBig(initMsg, OFFSET_LENGTH, INIT_MSGBODY_LENGTH);
		initMsg[INIT_OFFSET_PROTOCOL_VERSION+4] = PROTOCOL_VERSION;
		Tools::putIntBig(initMsg, INIT_OFFSET_CLIENT_ID+4, client->getClientId());
		Tools::putIntBig(initMsg, INIT_OFFSET_UDPPORT+4, client->remoteUDP.getPort());
		initMsg[INIT_OFFSET_CLIENT_TYPE+4] = CLIENT_TYPE;

		connection->sendData(initMsg);

		// wait for ack
		// | length (4 Bytes) | client id (4 Bytes) |
		const std::vector<uint8_t> ackMsg = Tools::readRawMSG(connection.get());
		if(ackMsg.empty() || ackMsg.size() != sizeof(int32_t) + sizeof(uint8_t)) {
			WARN("TCPConnection got wrong Ack-MSG.");
			client->postEvent(client->createEvent(EVENT_CONNECTION_FAILED,0));
			return;
		}
		uint8_t ackType_ = ackMsg[0];
		int32_t clientOrErrorId_ = Tools::getInt(ackMsg, sizeof(uint8_t));
		connected = true;
		if(ackType_ >= ACK_TYPE_ERR) {
			std::stringstream ss;
			ss << "Error connecting to the server: " << clientOrErrorId_;
			WARN(ss.str());
			connected = false;
			client->postEvent(client->createEvent(EVENT_CONNECTION_FAILED,0));
			return;
		}

		if(!client->setClientId(clientOrErrorId_)) {
			std::stringstream ss;
			ss << "Got the wrong ClientID from the server: " << clientOrErrorId_;
			WARN(ss.str());
			connected = false;
			client->postEvent(client->createEvent(EVENT_CONNECTION_FAILED,0));
			return;
		}
	}
	client->postEvent(client->createEvent(EVENT_TCP_CONNECTED, 0));

	while(connected && connection->isOpen()) {
		Utils::sleep(1);

		// send outgoing messages
		Message* msg = client->getNextTCPMSG();
		while(msg) {
			if(Tools::sendTCPMSG(connection.get(), msg)) {
				msg->dispose();
				msg = client->getNextTCPMSG();
			} else {
				client->asyncSend(msg);
				break;
			}
		}

		// receive incoming messages
		while(connection->getAvailableDataSize() > 0) {
			msg = Tools::readTCPMSG(connection.get(), client->msgTimeout);
			if(!msg) {
				WARN("Error while receiving message. Closing TCPHandler.");
				close();
			}
			client->distributeReceivedMSG(msg);
		}
		client->dispatchMessages(0xff, true);
	}
	connected = false;
	client->postEvent(client->createEvent(EVENT_TCP_DISCONNECTED, 0));
}

#endif /* MINSG_EXT_D3FACT */
