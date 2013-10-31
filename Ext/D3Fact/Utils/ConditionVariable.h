/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/


#ifdef MINSG_EXT_D3FACT

#ifndef CONDITIONVARIABLE_H_
#define CONDITIONVARIABLE_H_

namespace Util {
namespace Concurrency {
class Mutex;
class Semaphore;
}  // namespace Concurrency
}  // namespace Util

namespace D3Fact {

class ConditionVariable {
public:
	ConditionVariable(Util::Concurrency::Mutex* conition_mutex_);
	virtual ~ConditionVariable();

	void signal();
	void wait();
	void broadcast();
private:
	Util::Concurrency::Mutex* conition_mutex;
	Util::Concurrency::Semaphore* condition_semaphore;
	Util::Concurrency::Semaphore* waiters;
	Util::Concurrency::Mutex* waiters_mutex;
};

} /* namespace D3Fact */
#endif /* CONDITIONVARIABLE_H_ */
#endif /* MINSG_EXT_D3FACT */
