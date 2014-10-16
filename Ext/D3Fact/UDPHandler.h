/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT

#ifndef UDPHANDLER_H_
#define UDPHANDLER_H_

#include <mutex>
#include <thread>

namespace Util {
namespace Network {
class UDPNetworkSocket;
}
}

namespace D3Fact {

class ClientUnit;
class Message;

class UDPHandler {
public:
	UDPHandler(Util::Network::UDPNetworkSocket* socket_, ClientUnit* client_);
	~UDPHandler();

	void start();
	void close();

	void sendMSG(Message* msg_);
private:
	Util::Network::UDPNetworkSocket* socket;
	ClientUnit* client;
	std::mutex sendMutex;

	std::deque<Message* > sendQueue;

	std::thread thread;

	bool isMsgAvailable();
	Message* getNextMsg();

	void run();
};

}

#endif /* UDPHANDLER_H_ */
#endif /* MINSG_EXT_D3FACT */
