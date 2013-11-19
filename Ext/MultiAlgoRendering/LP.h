/*
	This file is part of the MinSG library extension MultiAlgoRendering.
	Copyright (C) 2009-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_MULTIALGORENDERING
#include "Dependencies.h"

#ifndef LP_H
#define LP_H

#include <vector>

#include <Util/Concurrency/UserThread.h>
#include <Util/Concurrency/Concurrency.h>
#include <Util/Concurrency/Lock.h>
#include <Util/Concurrency/Semaphore.h>
#include <Util/Concurrency/Mutex.h>
#include <Util/Macros.h>

COMPILER_WARN_PUSH
COMPILER_WARN_OFF_GCC(-Wredundant-decls)
#include <lp_lib.h>
COMPILER_WARN_POP

namespace MinSG
{

class LP : public Util::Concurrency::UserThread
{

public:

	LP ( uint32_t _nodeCount, uint32_t _algoCount, float * _errors, float * _times, double _targetTime );

	~LP();

	bool isStopped() {
		auto lock  = Util::Concurrency::createLock(* mutex );
		return stopped;
	}

	bool hasResult() {
		auto lock  = Util::Concurrency::createLock(* mutex );
		return resultComputed;
	}

	std::vector<int> getResult();

	virtual void run() override;

private:

	lprec * lp;
	uint32_t nodeCount;
	uint32_t algoCount;
	volatile double targetTime;
	REAL * result;
	volatile bool resultComputed;
	volatile bool stopped;

	Util::Concurrency::Mutex * mutex;
	Util::Concurrency::Semaphore * changed;

	void printMatrix();
	void printResult();
};

}

#endif // LP_H
#endif // MINSG_EXT_MULTIALGORENDERING
