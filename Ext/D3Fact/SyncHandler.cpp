/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT

#include <Util/Timer.h>

#include "Tools.h"
#include "Message.h"
#include "Session.h"
#include "SyncHandler.h"

#define MSGTYPE_PING 1
#define MSGTYPE_PONG 2

namespace D3Fact {

SyncHandler::SyncHandler(Session* session_, float syncPeriod_) : MessageHandler(ASYNC),
		session(session_), startTime(0), diff(0), ping(0), syncPeriod(syncPeriod_), synchronizing(false) { }

SyncHandler::~SyncHandler() { }

SyncHandler* SyncHandler::clone() const {
	return new SyncHandler(session.get(), syncPeriod);
}

double SyncHandler::getSyncTime() {
	if(!synchronizing && (Util::Timer::now() - startTime) > syncPeriod)
		sync();
	return Util::Timer::now() + diff;
}

void SyncHandler::handleMessage(Message* msg) {
	if(!synchronizing || msg->getType() != MSGTYPE_PONG || session == nullptr)
		return;

	ping = Util::Timer::now() - startTime;

	double serverTime = static_cast<double>(Tools::getLong(msg->getBody(), 0)) * 0.001 + ping * 0.5;

	double newDiff = serverTime - Util::Timer::now();
	diff= diff==0 ? newDiff : (diff*4.0 +newDiff)/5.0;

	synchronizing = false;

	msg->dispose();
}

void SyncHandler::sync() {
	Message* msg = session->createMessage(MSGTYPE_PING);
	startTime = Util::Timer::now();
	synchronizing = true;
	session->send(msg);
}

} /* namespace D3Fact */
#endif
