/*
	This file is part of the MinSG library extension MultiAlgoRendering.
	Copyright (C) 2009-2012 Ralf Petring <ralf@petring.net>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_MULTIALGORENDERING

#include "LP.h"
#include <list>
#include <iostream>

namespace MinSG {

using Util::Concurrency::createMutex;
using Util::Concurrency::createSemaphore;
using Util::Concurrency::Lock;
using Util::Concurrency::Semaphore;
using Util::Concurrency::Mutex;

static int __WINAPI abortfunction(lprec * /*lp*/, void * lp) {
	return reinterpret_cast<LP *>(lp)->isStopped();
}

LP::LP(uint32_t _nodeCount, uint32_t _algoCount, float * _errors, float * _times, double _targetTime) :
			UserThread(),
			lp(nullptr),
			nodeCount(_nodeCount),
			algoCount(_algoCount),
			targetTime(_targetTime),
			result(nullptr),
			resultComputed(false),
			stopped(false),
			mutex(createMutex())
	{
	result = new REAL[1 + (nodeCount + 1) + algoCount * nodeCount ]; // 1 + #rows + #cols
	REAL * times = new REAL[1 + nodeCount * algoCount];
	REAL * errors = new REAL[1 + nodeCount * algoCount];
	std::copy(_errors, _errors + nodeCount * algoCount, errors + 1);
	std::copy(_times, _times + nodeCount * algoCount, times + 1);

	lp = make_lp(0, nodeCount * algoCount);
	if(lp == nullptr)
		WARN("could not create LP");
	set_minim(lp);
	resize_lp(lp, nodeCount + 1, nodeCount * algoCount);

	for(int_fast32_t i = 1; i <= static_cast<int_fast32_t>(algoCount * nodeCount); ++i)
		set_binary(lp, i, true);

	set_obj_fn(lp, errors);

	int count = algoCount;
	REAL * row = new REAL[count];
	std::fill(row, row + count, 1);
	auto colNo = new int[count];
	for(int_fast32_t i = 0; i < count; ++i)
		colNo[i] = i + 1;
	set_add_rowmode(lp, true);
	for(uint_fast32_t i = 0; i < nodeCount; ++i) {  // add one algo per node constraints
		add_constraintex(lp, count, row, colNo, EQ, 1);
		if(i + 1 < nodeCount) {
			for(int * j = colNo; j < colNo + count; ++j)
				*j += count;
		}
	}
	add_constraint(lp, times, LE, targetTime);    // add main constraint
	set_add_rowmode(lp, false);

	put_abortfunc(lp, abortfunction, this);

	delete [] row;
	delete [] colNo;
	delete [] times;
	delete [] errors;
	
	set_verbose(lp, IMPORTANT);

	printMatrix();

	start();
}

void LP::printMatrix() {
#ifndef NDEBUG
	std::cerr << "LP-Matrix: +++++++++++++++++++++++\n";
	for(unsigned r = 0; r < 1 + 1 + nodeCount; r++) {
		for(unsigned c = 1; c < 1 + nodeCount * algoCount; c++) {
			std::cerr << get_mat(lp, r, c) << " \t ";
		}
		if(r != 0) {
			int ct = get_constr_type(lp, r);
			std::cerr << (ct == 1 ? "<=" : (ct == 2 ? ">=" : (ct == 3 ? "==" : "?"))) << " \t " << get_rh(lp, r);
		}
		std::cerr << std::endl;
	}
	std::cerr << "LP-Matrix: -----------------------\n";
#endif
}

void LP::printResult() {
#ifndef NDEBUG
	std::cerr << "LP-Result: +++++++++++++++++++++++\n";
	std::cerr << result[0] << std::endl;
	for(unsigned i = 1; i <= nodeCount + 1; i++)
		std::cerr << result[i] << " \t ";
	std::cerr << std::endl;
	for(unsigned r = 0; r < nodeCount; r++) {
		for(unsigned c = 0; c < algoCount; c++)
			std::cerr << result[r * algoCount + c + (nodeCount + 1 + 1)] << " \t ";
		std::cerr << std::endl;
	}
	std::cerr << "LP-Result: -----------------------\n";
#endif
}

LP::~LP() {
	if(isActive())
		join();
	delete_lp(lp);
}

std::vector<int> LP::getResult() {
	auto lock = Util::Concurrency::createLock(*mutex);
	std::vector<int> ret;
	ret.reserve(nodeCount);
	printResult();

	for(unsigned r = 0; r < nodeCount; r++) {
		bool found = false;
		for(unsigned c = 0; c < algoCount; c++) {
			if(result[r * algoCount + c + (nodeCount + 1 + 1)] == 1) {
				if(found)
					WARN("error: more than one algo selected");
				else {
					ret.push_back(c);
					found = true;
				}
			}
		}
		if(!found)
			WARN("error: no algo selected");
	}
	return ret;
}

void LP::run() {

	{
		auto lock = Util::Concurrency::createLock(*mutex);

		set_rh(lp, nodeCount + 1, targetTime);
	}

	solve(lp);

	{
		auto lock = Util::Concurrency::createLock(*mutex);
		get_primal_solution(lp, result);
		resultComputed = true;
	}
}

}

#endif // MINSG_EXT_MULTIALGORENDERING
