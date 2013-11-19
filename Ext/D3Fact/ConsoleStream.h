/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT


#ifndef CONSOLESTREAM_H_
#define CONSOLESTREAM_H_

#include <Util/TypeNameMacro.h>
#include <Util/ReferenceCounter.h>
#include <Util/References.h>

#include <iostream>

namespace D3Fact {

class Session;
class IOStreamHandler;

class ConsoleStream : public Util::ReferenceCounter<ConsoleStream> {
	PROVIDES_TYPE_NAME(ConsoleStream)
public:
	ConsoleStream();
	virtual ~ConsoleStream();

	bool open(Session* session_);
	void close();

	bool isOpen() { return is_open; };

	void write(const std::string& text);
	std::string read();
private:
	Util::Reference<Session> session;
	Util::Reference<IOStreamHandler> handler;

	bool is_open;

	std::istream* instream;
	std::ostream* outstream;

	int32_t inStreamId;
	int32_t outStreamId;
};

} /* namespace D3Fact */
#endif /* CONSOLESTREAM_H_ */
#endif /* MINSG_EXT_D3FACT */
