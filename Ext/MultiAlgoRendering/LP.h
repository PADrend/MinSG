/*
	This file is part of the MinSG library extension MultiAlgoRendering.
	Copyright (C) 2009-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_MULTIALGORENDERING
#include "Dependencies.h"

#ifndef MAR_LP_H
#define MAR_LP_H

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include <Util/Macros.h>

COMPILER_WARN_PUSH
COMPILER_WARN_OFF_GCC(-Wredundant-decls)
#include <lp_lib.h>
COMPILER_WARN_POP


namespace MinSG
{

class LP
{

public:

	MINSGAPI LP ( uint32_t _nodeCount, uint32_t _algoCount, float * _errors, float * _times, double _targetTime );

	MINSGAPI ~LP();

	bool isStopped() {
		std::lock_guard<std::mutex> lock(mutex);
		return stopped;
	}

	bool hasResult() {
		std::lock_guard<std::mutex> lock(mutex);
		return resultComputed;
	}

	MINSGAPI std::vector<int> getResult();

	MINSGAPI void run();

private:

	lprec * lp;
	uint32_t nodeCount;
	uint32_t algoCount;
	volatile double targetTime;
	REAL * result;
	volatile bool resultComputed;
	volatile bool stopped;

	std::mutex mutex;
	std::condition_variable changed;
	std::thread thread;

	MINSGAPI void printMatrix();
	MINSGAPI void printResult();
};

}

#endif // LP_H
#endif // MINSG_EXT_MULTIALGORENDERING
