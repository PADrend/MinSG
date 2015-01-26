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
#include <Util/Network/NetworkUDP.h>

#include "ClientUnit.h"
#include "Message.h"
#include "Tools.h"
#include "UDPHandler.h"

using namespace D3Fact;
using namespace Util;
using namespace Util::Network;

UDPHandler::UDPHandler(UDPNetworkSocket* socket_, ClientUnit* client_) :
		socket(socket_), client(client_), sendMutex() {
}

UDPHandler::~UDPHandler() {
	close();
	delete socket;
}

void UDPHandler::start() {
	thread = std::thread(std::bind(&UDPHandler::run, this));
}

void UDPHandler::close() {
	socket->close();
	thread.join();
}

void UDPHandler::sendMSG(Message* msg_) {
	std::lock_guard<std::mutex> lock(sendMutex);
	sendQueue.push_back(msg_);
}

bool UDPHandler::isMsgAvailable() {
	std::lock_guard<std::mutex> lock(sendMutex);
	return !sendQueue.empty();
}

Message* UDPHandler::getNextMsg() {
	std::lock_guard<std::mutex> lock(sendMutex);
	Message* msg = sendQueue.front();
	sendQueue.pop_front();
	return msg;
}

void UDPHandler::run() {
	socket->open();
	while(socket->isOpen()) {
		Utils::sleep(1);

		// receive incoming messages
		Message* msg = Tools::readUDPMSG(socket);
		while(msg) {
			client->distributeReceivedMSG(msg);
			msg = Tools::readUDPMSG(socket);
		};

		// send outgoing messages
		while(isMsgAvailable()) {
			msg = getNextMsg();
			if(Tools::sendUDPMSG(socket, msg)) {
				msg->dispose();
			} else {
				client->asyncSend(msg);
				break;
			}
		}
	}
}

#endif /* MINSG_EXT_D3FACT */
