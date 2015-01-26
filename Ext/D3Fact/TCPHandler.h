/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT

#ifndef TCPHANDLER_H_
#define TCPHANDLER_H_

#include <thread>

namespace D3Fact {

class ClientUnit;

class TCPHandler {
public:
	TCPHandler(ClientUnit* client_);
	~TCPHandler();

	void start();
	void close();
	bool isConnected() { return connected; }
private:
	ClientUnit* client;
	bool connected;
	std::thread thread;

	void run();
};

}

#endif /* TCPHANDLER_H_ */
#endif /* MINSG_EXT_D3FACT */
