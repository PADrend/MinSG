/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/


#ifdef MINSG_EXT_D3FACT

#include "ConditionVariable.h"

#include <Util/Concurrency/Concurrency.h>
#include <Util/Concurrency/Mutex.h>
#include <Util/Concurrency/Lock.h>
#include <Util/Concurrency/Semaphore.h>

namespace D3Fact {
using namespace Util::Concurrency;

ConditionVariable::ConditionVariable(Mutex* conition_mutex_) :
		conition_mutex(conition_mutex_), condition_semaphore(createSemaphore()), waiters(createSemaphore()), waiters_mutex(createMutex()) {
}

ConditionVariable::~ConditionVariable() {
	broadcast();
	delete condition_semaphore;
	delete waiters;
	delete waiters_mutex;
}

void ConditionVariable::signal() {
	auto lock = Util::Concurrency::createLock(*waiters_mutex);
	if(waiters->tryWait())
		condition_semaphore->post();
}

void ConditionVariable::wait() {
	{
		auto lock = Util::Concurrency::createLock(*waiters_mutex);
		waiters->post();
		conition_mutex->unlock();
	}
	condition_semaphore->wait();
	conition_mutex->lock();
}

void ConditionVariable::broadcast() {
	auto lock = Util::Concurrency::createLock(*waiters_mutex);
	while(waiters->tryWait())
		condition_semaphore->post();
}

} /* namespace D3Fact */

#endif /* MINSG_EXT_D3FACT */
