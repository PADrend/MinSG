/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT

#include "Message.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>
#include <iostream>

using namespace D3Fact;
using namespace Util;


std::deque< Message* > Message::CACHE = std::deque< Message* >();

static std::mutex & getCacheMutex() {
	static std::mutex mutex;
	return mutex;
}

Message::Message() : clientID(0), sessionID(0), orderID(0), type(0), body(), off(0), len(0), protocol(TCP), disposed(false) {}

Message::~Message() {}

bool Message::operator <(const Message& b) const {
	if( orderID < b.orderID )
		return true;
	else if( orderID > b.orderID )
		return false;
	else if( protocol == TCP )
		return true;
	else
		return false;
}
bool Message::operator >(const Message& b) const {
	if( orderID > b.orderID )
		return true;
	else if( orderID < b.orderID )
		return false;
	else if( b.protocol == TCP )
		return true;
	else
		return false;
}

void Message::set(int32_t clientID_, int32_t sessionID_, int64_t orderID_, int32_t type_) {
	clientID = clientID_;
	sessionID = sessionID_;
	orderID = orderID_;
	type = type_;
	protocol = UDP;
}

void Message::set(int32_t sessionID_, int64_t orderID_, int32_t type_) {
	clientID = 0;
	sessionID = sessionID_;
	orderID = orderID_;
	type = type_;
	protocol = TCP;
}

void Message::set(const std::vector<uint8_t> & body_) {
	body = body_;
	off = 0;
	len = body.size();
}

void Message::set(const std::vector<uint8_t> & body_, size_t off_, size_t len_) {
	body = body_;
	off = off_;
	len = len_;
}

void Message::dispose() {
	body.resize(0);
	body.shrink_to_fit();
	off = 0;
	len = 0;
	if(disposed)
		return;
	disposed=true;
	std::lock_guard<std::mutex> lock(getCacheMutex());
	CACHE.push_back(this);
}

Message* Message::get() {
	std::lock_guard<std::mutex> lock(getCacheMutex());
	if(CACHE.empty())
		return new Message();
	Message* msg = CACHE.front();
	CACHE.pop_front();
	if(!msg)
		return new Message();
	msg->disposed=false;
	return msg;
}

Message* Message::get(int32_t sessionID_, int64_t orderID_, int32_t type_, const std::vector<uint8_t> & body_) {
	Message* msg = get();
	msg->set(sessionID_, orderID_, type_);
	msg->set(body_);
	return msg;
}
Message* Message::get(int32_t sessionID_, int64_t orderID_, int32_t type_, const std::vector<uint8_t> & body_, size_t off_, size_t len_) {
	Message* msg = get();
	msg->set(sessionID_, orderID_, type_);
	msg->set(body_, off_, len_);
	return msg;
}
Message* Message::get(int32_t clientID_, int32_t sessionID_, int64_t orderID_, int32_t type_, const std::vector<uint8_t> & body_) {
	Message* msg = get();
	msg->set(clientID_, sessionID_, orderID_, type_);
	msg->set(body_);
	return msg;
}
Message* Message::get(int32_t clientID_, int32_t sessionID_, int64_t orderID_, int32_t type_, const std::vector<uint8_t> & body_, size_t off_, size_t len_) {
	Message* msg = get();
	msg->set(clientID_, sessionID_, orderID_, type_);
	msg->set(body_, off_, len_);
	return msg;
}


Message* Message::clone() {
	Message* msg= Message::get(clientID, sessionID, orderID, type, body, off, len);
	msg->protocol = protocol;
	return msg;
}

#endif /* MINSG_EXT_D3FACT */
