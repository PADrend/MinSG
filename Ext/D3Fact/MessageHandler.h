/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT

#ifndef MESSAGEHANDLER_H_
#define MESSAGEHANDLER_H_

#include <deque>

#include <Util/TypeNameMacro.h>
#include <Util/References.h>
#include <Util/ReferenceCounter.h>
#include <Util/AttributeProvider.h>

namespace Util {
namespace Concurrency {
class Mutex;
}
}

namespace D3Fact {

class Message;
class WorkerThread;

class MessageHandler :
	public Util::ReferenceCounter<MessageHandler>,
	public Util::AttributeProvider {
	PROVIDES_TYPE_NAME(MessageHandler)
public:
	enum Mode_t { SYNC, ASYNC };

	MessageHandler(Mode_t mode_);
	virtual ~MessageHandler();

	virtual MessageHandler* clone() const { return nullptr; };

	void addMessage(Message* msg);
	void handleMessages();

	Mode_t getMode() { return mode; }
protected:
	Message* nextMessage();

	virtual void handleMessage(Message* msg) = 0;
private:
	Mode_t mode;
	Util::Concurrency::Mutex* queueMutex;
	std::deque<Message*> msgQueue;
	WorkerThread* worker;
};

} /* namespace D3Fact */
#endif /* MESSAGEHANDLER_H_ */
#endif /* MINSG_EXT_D3FACT */
