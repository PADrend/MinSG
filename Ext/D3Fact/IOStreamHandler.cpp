/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_D3FACT

#include "IOStreamHandler.h"
#include "Session.h"
#include "Message.h"
#include "Tools.h"
#include "Utils/SyncBuffer.h"
#include "Utils/StreamManipulators.h"

#include <Util/Macros.h>
#include <Util/References.h>

#include <sstream>
#include <thread>

#define READ_BUFFER_SIZE 4096

namespace D3Fact {

class IOStreamHandler::OutStreamer {
public:
	OutStreamer(IOStreamHandler* handler_, std::istream* stream_, uint32_t streamId_) :
		handler(handler_), stream(stream_), streamId(streamId_),
		thread(std::bind(&IOStreamHandler::OutStreamer::run, this)) {
	}

	void join() {
		thread.join();
	}
private:
	void run();

	Util::WeakPointer<IOStreamHandler> handler;
	std::istream* stream;
	uint32_t streamId;
	std::thread thread;
};

void IOStreamHandler::OutStreamer::run() {

	char buffer[READ_BUFFER_SIZE];
	StreamMessage_t type = STREAM_HEADER;
	while(!stream->eof() ) {
		Message* msg = handler->session->createMessage(type);
		std::vector<uint8_t> body = msg->accessBody();
		Tools::appendIntBig(body, streamId);
		if(type == STREAM_HEADER)
			Tools::appendIntBig(body, -1); // stream length not supported

		size_t read = stream->read(buffer, READ_BUFFER_SIZE).gcount();
		size_t body_size = body.size();
		body.resize(body_size + read);
		char* begin = reinterpret_cast<char *>(body.data()) + body_size;
		std::copy(buffer, buffer + read, begin);

		msg->set(body);
		handler->session->send(msg);
		if(type == STREAM_HEADER)
			type = STREAM_INTERMEDIATE;
	}
	if(handler->session->isClosed()) {
		WARN("Closed session with open streams.");
		return;
	}

	Message* msg = handler->session->createMessage(STREAM_TAIL);
	std::vector<uint8_t> body = msg->accessBody();
	Tools::appendIntBig(body, streamId);
	msg->set(body);
	handler->session->send(msg);
}

IOStreamHandler::IOStreamHandler(Session* session_) :
	MessageHandler(ASYNC), session(session_), instreams(), mutex() {
}

IOStreamHandler::~IOStreamHandler() {
	{
		std::lock_guard<std::mutex> lock(mutex);
		for(auto & in : instreams)
			delete in.second;
		instreams.clear();
	}
	for(auto & out : outstreams)
		delete out.second;
}

std::istream* IOStreamHandler::request(int32_t streamId) {
	auto buf = new SyncBuffer();
	auto stream = new std::iostream(buf);
	std::lock_guard<std::mutex> lock(mutex);
	instreams[streamId] = buf;
	return stream;
}

void IOStreamHandler::cancelStream(int32_t streamId) {
	std::lock_guard<std::mutex> lock(mutex);
	if(instreams.count(streamId)>0) {
		instreams[streamId]->close();
//		instreams.erase(streamId);
//		delete instreams[streamId];
	}
}


std::ostream* IOStreamHandler::send(int32_t streamId) {
	if(outstreams.count(streamId)>0)
		return nullptr;

	auto stream = new std::iostream(new SyncBuffer());
	outstreams[streamId] = new OutStreamer(this, stream, streamId);

	return stream;
}

void IOStreamHandler::waitFor(int32_t streamId) {
	if(outstreams.count(streamId)>0)
		outstreams[streamId]->join();
}


void IOStreamHandler::handleMessage(Message* msg) {
	if(msg->getType() < STREAM_HEADER || msg->getType() > STREAM_ERROR) {
		WARN("IOStreamHandler can only handle messages of type 200,201,202,203.");
		return;
	}

	int32_t streamId = Tools::getInt(msg->getBody(), 0);

	std::lock_guard<std::mutex> lock(mutex);
	if(instreams.count(streamId)==0) {
		std::stringstream ss;
		ss << "There is no stream request for stream '" << streamId << "'.";
		WARN(ss.str());
	} else if(msg->getType() == STREAM_HEADER) {
		// | contentID(int) | length(int) | streamdata(*) |
		const uint8_t * dataBegin = msg->getBody().data() + 2*sizeof(int32_t);
		const std::size_t dataSize = msg->getBody().size() - 2*sizeof(int32_t);

		if(dataSize > 0)
			instreams[streamId]->sputn(reinterpret_cast<const char*>(dataBegin), dataSize);
	} else if(msg->getType() == STREAM_INTERMEDIATE) {
		// | contentID(int) | streamdata(*) |

		const uint8_t * dataBegin = msg->getBody().data() + sizeof(int32_t);
		const std::size_t dataSize = msg->getBody().size() - sizeof(int32_t);
		if(dataSize > 0)
			instreams[streamId]->sputn(reinterpret_cast<const char*>(dataBegin), dataSize);
	} else if(msg->getType() == STREAM_TAIL) {
		// | contentID(int) | streamdata(*) |
		const uint8_t * dataBegin = msg->getBody().data() + sizeof(int32_t);
		const std::size_t dataSize = msg->getBody().size() - sizeof(int32_t);
		SyncBuffer* stream = instreams[streamId];

		if(dataSize > 0)
			stream->sputn(reinterpret_cast<const char*>(dataBegin), dataSize);
		stream->close();

//		instreams.erase(streamId);
//		delete stream;
	} else if(msg->getType() == STREAM_ERROR) {
		// | contentID(int) |
		std::stringstream ss;
		ss << "Error while receiving stream '" << streamId << "'.";
		WARN(ss.str());
		SyncBuffer* stream = instreams[streamId];

		stream->close();
//		instreams.erase(streamId);
//		delete stream;
	}
	msg->dispose();
}

} /* namespace D3Fact */

#endif /* MINSG_EXT_D3FACT */
