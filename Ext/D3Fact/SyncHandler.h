/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT

#ifndef SYNCHANDLER_H_
#define SYNCHANDLER_H_

#include "MessageHandler.h"

#include <Util/References.h>

namespace D3Fact {

class Session;

class SyncHandler: public D3Fact::MessageHandler {
	PROVIDES_TYPE_NAME(SyncHandler)
public:
	SyncHandler(Session* session_, float syncPeriod_=1.0);
	virtual ~SyncHandler();

	virtual SyncHandler* clone() const;

	double getSyncTime();
	float getPing() const { return ping; }
protected:
	virtual void handleMessage(Message* msg);
private:
	void sync();
	Util::Reference<Session> session;
	double startTime;
	double diff;
	double ping;
	float syncPeriod;
	bool synchronizing;
};

} /* namespace D3Fact */
#endif /* SYNCHANDLER_H_ */
#endif
