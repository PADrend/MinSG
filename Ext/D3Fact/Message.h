/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <Util/References.h>
#include <cstdint>
#include <deque>
#include <functional>
#include <vector>

#define OFFSET_LENGTH 0
#define OFFSET_TCP_SESSIONID 4
#define OFFSET_TCP_ORDER 8
#define OFFSET_TCP_MSGTYPE 16
#define OFFSET_TCP_BODY 20

#define OFFSET_UDP_CLIENTID 4
#define OFFSET_UDP_SESSIONID 8
#define OFFSET_UDP_ORDER 12
#define OFFSET_UDP_MSGTYPE 20
#define OFFSET_UDP_BODY 24

#define LENGTH_TCP_HEADER 16
#define LENGTH_UDP_HEADER 20

#define INIT_OFFSET_PROTOCOL_VERSION 0
#define INIT_OFFSET_CLIENT_ID 1
#define INIT_OFFSET_UDPPORT 5
#define INIT_OFFSET_CLIENT_TYPE 9
#define INIT_MSGBODY_LENGTH 10

#define INITACK_MSGBODY_LENGTH 4

#define MSGTYPE_START_NEW_SESSION -10
#define MSGTYPE_ACK_NEW_SESSION -11
#define MSGTYPE_ERROR_NEW_SESSION -12

#define MSGTYPE_CLOSE_SESSION -20
#define MSGTYPE_ACK_CLOSE_SESSION -21

namespace D3Fact {

class Message  {
public:
	enum protocol_t {
		TCP, UDP
	};

	Message();
	virtual ~Message();

	int32_t getClientId() const { return clientID; }
	int32_t getSession() const { return sessionID; }
	int64_t getOrder() const { return orderID; }
	int32_t getType() const { return type; }
	std::vector<uint8_t> & accessBody() { return body; }
	const std::vector<uint8_t> & getBody() const { return body; }
	int32_t getBodyOffset() const { return off; }
	int32_t getBodyLength() const { return len; }
	protocol_t getProtocol() const { return protocol; }

	bool operator <(const Message& b) const;
	bool operator >(const Message& b) const;

	void set(int32_t clientID_, int32_t sessionID_, int64_t orderID_, int32_t type_);
	void set(int32_t sessionID_, int64_t orderID_, int32_t type_);
	void set(const std::vector<uint8_t> & body_);
	void set(const std::vector<uint8_t> & body_, size_t off_, size_t len_);
	void setOrder(int64_t orderID_) { orderID = orderID_; }

	void dispose();

	static Message* get();
	static Message* get(int32_t sessionID_, int64_t orderID_, int32_t type_, const std::vector<uint8_t> & body_);
	static Message* get(int32_t sessionID_, int64_t orderID_, int32_t type_, const std::vector<uint8_t> & body_, size_t off_, size_t len_);
	static Message* get(int32_t clientID_, int32_t sessionID_, int64_t orderID_, int32_t type_, const std::vector<uint8_t> & body_);
	static Message* get(int32_t clientID_, int32_t sessionID_, int64_t orderID_, int32_t type_, const std::vector<uint8_t> & body_, size_t off_, size_t len_);

	Message* clone();

private:
	int32_t clientID;
	int32_t sessionID;
	int64_t orderID;
	int32_t type;
	std::vector<uint8_t> body;
	int32_t off;
	int32_t len;
	protocol_t protocol;
	bool disposed;

	static std::deque< Message* > CACHE;
};

}

#endif /* MESSAGE_H_ */
#endif /* MINSG_EXT_D3FACT */
