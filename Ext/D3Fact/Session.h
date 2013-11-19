/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT

#ifndef SESSION_H_
#define SESSION_H_

#include <cstdint>
#include <queue>

#include <Util/TypeNameMacro.h>
#include <Util/ReferenceCounter.h>
#include <Util/References.h>

namespace Util {
namespace Concurrency {
class Mutex;
}
}

namespace D3Fact {

template<typename Type, typename Compare = std::greater<Type> >
struct pcomp {
	bool operator()(const Type *x, const Type *y) const
		{ return Compare()(*x, *y); }
};

class Message;
class ClientUnit;
class MessageDispatcher;

class Session : public Util::ReferenceCounter<Session> {
	PROVIDES_TYPE_NAME(Session)
public:
	Session(int32_t sessionId_, ClientUnit* client_);
	virtual ~Session();

	int32_t getSessionId() { return sessionId; }
	ClientUnit* getClient() { return client.get(); }

	Message* createMessage(int32_t type, size_t bodySize=0, int protocol=0);

	bool hasReceivedMSG();
	Message* receiveMSG();
	void received(Message* msg_);

	void send(Message* msg);

	void close();
	bool isClosed() { return closed; }

	MessageDispatcher* getDispatcher() { return dispatcher.get(); }
private:
	int32_t sessionId;
	bool closed;
	Util::WeakPointer<ClientUnit> client;
	int64_t receivedMSGID;
	int64_t sendMSGID;
	Util::Reference<MessageDispatcher> dispatcher;

	Util::Concurrency::Mutex* receivedMutex;
	std::priority_queue<Message*, std::vector<Message*>, pcomp<Message> > receivedQueue;
};

}

#endif /* SESSION_H_ */
#endif /* MINSG_EXT_D3FACT */
