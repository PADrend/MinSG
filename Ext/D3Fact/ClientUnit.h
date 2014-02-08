/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT

#ifndef CLIENTUNIT_H_
#define CLIENTUNIT_H_

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <deque>

#include <Util/TypeNameMacro.h>
#include <Util/Network/Network.h>
#include <Util/References.h>
#include <Util/ReferenceCounter.h>

namespace Util {
class GenericAttribute;
class GenericAttributeMap;
class TemporaryDirectory;
class FileName;
namespace Network {
class TCPConnection;
}
namespace Concurrency {
class Mutex;
}
}

namespace D3Fact {

class Session;
class Message;
class TCPHandler;
class UDPHandler;

class ClientUnit : public Util::ReferenceCounter<ClientUnit> {
	PROVIDES_TYPE_NAME(ClientUnit)
public:
	ClientUnit(const std::string & remoteHost_, uint16_t remoteTCP_, uint16_t remoteUDP_, uint8_t maxConnections_=1, uint32_t msgTimeout_=2000);
	virtual ~ClientUnit();

	bool connect();

	int32_t startSession();
	Session * getSession(int32_t sessionId_);
	void closeSession(Session * session_);
	void dispatchMessages(uint32_t maxWorkload=0xffffffff, bool async=false);

	void distributeReceivedMSG(Message * msg_);
	void handleSession0Messages(Message * msg_);
	void asyncSend(Message * msg_);

	int32_t getClientId() const { return clientId; }
	bool isConnected();
	void close();

	Message* getNextTCPMSG();

	const Util::FileName & getTempPath() const;

	Util::GenericAttribute* pollEvent();
	void postEvent(Util::GenericAttribute* event);
	Util::GenericAttributeMap* createEvent(const std::string& eventId, int32_t sessionId=0);

	static ClientUnit* getClient(int32_t id);
private:
	friend class TCPHandler;
	friend class UDPHandler;

	Util::Concurrency::Mutex* sessionMutex;
	Util::Concurrency::Mutex* sendMutex;
	Util::Concurrency::Mutex* connectMutex;

	int32_t clientId;
	bool connected;

	Util::Network::IPv4Address remoteTCP;
	Util::Network::IPv4Address remoteUDP;

	uint8_t maxConnections;
	uint32_t msgTimeout;

	UDPHandler* udpHandler;
	std::vector<TCPHandler*> tcpPool;
	std::map<int32_t, Util::Reference<Session> > sessionMap;
	std::deque< Message* > sendTCP;
	std::set<int32_t> openSessionRequests;

	std::deque<Util::GenericAttribute*> eventQueue;
	Util::Concurrency::Mutex* eventMutex;

	Util::Reference< Util::TemporaryDirectory > tempDir;

	static std::map<int32_t, Util::Reference<ClientUnit> > clients;

	bool setClientId(int32_t id);
};

}

#endif /* CLIENTUNIT_H_ */
#endif /* MINSG_EXT_D3FACT */
