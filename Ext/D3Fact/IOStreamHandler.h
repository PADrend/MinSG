/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_D3FACT

#ifndef SINGLESTREAMHANDLER_H_
#define SINGLESTREAMHANDLER_H_

#include "MessageHandler.h"

#include <Util/References.h>

#include <iostream>
#include <map>

enum StreamMessage_t {
	STREAM_HEADER = 200,
	STREAM_INTERMEDIATE = 201,
	STREAM_TAIL = 202,
	STREAM_ERROR = 203
};

namespace Util {
namespace Concurrency {
class Thread;
class Mutex;
}
}

namespace D3Fact {

class Session;
class Message;
class SyncBuffer;

class IOStreamHandler: public D3Fact::MessageHandler {
public:

	IOStreamHandler(Session* session);
	virtual ~IOStreamHandler();

	std::istream* request(int32_t streamId);
	void cancelStream(int32_t streamId);

	std::ostream* send(int32_t streamId);
	void waitFor(int32_t streamId);

	Session* getSession() { return session.get(); }
protected:
	virtual void handleMessage(Message* msg) override;
private:
	class OutStreamer;

	Util::Reference<Session> session;
	std::map<int32_t, SyncBuffer*> instreams;
	std::map<int32_t, OutStreamer*> outstreams;
	Util::Concurrency::Mutex* mutex;
};

} /* namespace D3Fact */
#endif /* SINGLESTREAMHANDLER_H_ */
#endif /* MINSG_EXT_D3FACT */
