/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT

#ifndef MESSAGEDISPATCHER_H_
#define MESSAGEDISPATCHER_H_

#include <cstdint>
#include <map>

#include <Util/TypeNameMacro.h>
#include <Util/References.h>
#include <Util/ReferenceCounter.h>

namespace Util {
namespace Concurrency {
class Mutex;
}
}

namespace D3Fact {

class Session;
class Message;
class MessageHandler;

class MessageDispatcher : public Util::ReferenceCounter<MessageDispatcher>  {
	PROVIDES_TYPE_NAME(MessageDispatcher)
public:
	MessageDispatcher(Session* session_);
	virtual ~MessageDispatcher();

	void dispatch(uint32_t maxWorkload=0xffffffff, bool async=false);

	void registerHandler(MessageHandler* msgHandler, int32_t type);
	void unregisterHandler(int32_t type);
	MessageHandler* getHandler(int32_t type);

	bool hasHandler(int32_t type);

	Session* getSession() { return session.get(); }

	void dispose();
private:
	Util::Reference<Session> session;
	Util::Concurrency::Mutex* mutex;
	Util::Concurrency::Mutex* sessionMutex;

	typedef std::map<int32_t, Util::Reference<MessageHandler> > HandlerMap_t;
	HandlerMap_t handler;
};

} /* namespace D3Fact */
#endif /* MESSAGEDISPATCHER_H_ */
#endif /* MINSG_EXT_D3FACT */
