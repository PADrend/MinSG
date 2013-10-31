/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT

#include "ConsoleStream.h"
#include "ClientUnit.h"
#include "Session.h"
#include "MessageDispatcher.h"
#include "Message.h"
#include "Tools.h"
#include "IOStreamHandler.h"

#include <Util/IO/FileUtils.h>
#include <Util/IO/FileName.h>
#include <Util/Macros.h>

#include <sstream>

namespace D3Fact {

enum MessageType_t {
	MSGTYPE_COMMAND_OPEN_CONSOLE = 7000,
	MSGTYPE_COMMAND_CONSOLE_OPENED = 7001,
	MSGTYPE_COMMAND_OPEN_CONSOLE_ERROR = 7002,
	MSGTYPE_COMMAND_CLOSE_CONSOLE = 7010,
	MSGTYPE_COMMAND_CONSOLE_CLOSED = 7011,
	MSGTYPE_COMMAND_CLOSE_CONSOLE_ERROR = 7012,
	MSGTYPE_COMMAND_RUN_COMMAND = 7020,
	MSGTYPE_COMMAND_RUN_COMMAND_ERROR = 7022,
};

ConsoleStream::ConsoleStream() : Util::ReferenceCounter<ConsoleStream>(),
		session(), handler(), is_open(false), instream(nullptr), outstream(nullptr), inStreamId(0), outStreamId(1) {}

ConsoleStream::~ConsoleStream() {
	close();
	delete instream;
	delete outstream;
}

bool ConsoleStream::open(Session* session_) {
	/*{
		std::stringstream ss;
		ss << "d3fact://" << session->getClient()->getClientId() << "/" << session->getSessionId() << "/0/console:/0";
		instream = Util::FileUtils::openForReading(Util::FileName(ss.str()));
	}
	{
		std::stringstream ss;
		ss << "d3fact://" << session->getClient()->getClientId() << "/" << session->getSessionId() << "/1/console:/0";
		outstream = Util::FileUtils::openForWriting(Util::FileName(ss.str()));
	}

	is_open = (outstream != nullptr && instream != nullptr);
	return is_open;*/
	this->session = session_;

	if(!this->session->getDispatcher()->hasHandler(STREAM_HEADER)) {
		handler = new IOStreamHandler(this->session.get());
		this->session->getDispatcher()->registerHandler(handler.get(), STREAM_HEADER);
		this->session->getDispatcher()->registerHandler(handler.get(), STREAM_INTERMEDIATE);
		this->session->getDispatcher()->registerHandler(handler.get(), STREAM_TAIL);
		this->session->getDispatcher()->registerHandler(handler.get(), STREAM_ERROR);
	} else {
		handler = dynamic_cast<IOStreamHandler*>(this->session->getDispatcher()->getHandler(STREAM_HEADER));
		if(handler.isNull()) {
			std::stringstream ss;
			ss << "Could not register IOStreamHandler for ConsoleStream at Session " << this->session->getSessionId();
			WARN(ss.str());
			return false;
		}
	}

	instream = handler->request(inStreamId);

	// send message OPEN_CONSOLE [streamId(int)]
	Message* msg = this->session->createMessage(MSGTYPE_COMMAND_OPEN_CONSOLE);
	std::vector<uint8_t> body = msg->accessBody();
	Tools::appendIntBig(body, inStreamId);
	msg->set(body);
	this->session->send(msg);

	is_open = (instream != nullptr);

	return is_open;
}

void ConsoleStream::close() {
	if(!is_open)
		return;

	if(session.isNotNull() && !session->isClosed()) {
		// send message CONSOLE_CLOSED
		Message* msg = session->createMessage(MSGTYPE_COMMAND_CONSOLE_CLOSED);
		session->send(msg);
	}

	delete instream;
	instream = nullptr;
	is_open = false;
	session = nullptr;
}

void ConsoleStream::write(const std::string& text) {
	if(!is_open)
		return;
	//outstream->write(text.c_str(), text.size());

	if(session.isNotNull() && !session->isClosed()) {
		// send message MSGTYPE_COMMAND_RUN_COMMAND [text(string)]
		Message* msg = session->createMessage(MSGTYPE_COMMAND_RUN_COMMAND);
		std::vector<uint8_t> body = msg->accessBody();
		Tools::appendString(body, text);
		msg->set(body);
		session->send(msg);
	}

}

std::string ConsoleStream::read() {
	if(!is_open)
		return "";
	char buffer[4096];
	std::streamsize len = instream->readsome(buffer, 4096);
	return std::string(buffer, len);
}

} /* namespace D3Fact */
#endif /* MINSG_EXT_D3FACT */
