/*
	This file is part of the MinSG library extension Profiling.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "Profiler.h"
#include "Logger.h"
#include <Util/GenericAttribute.h>
#include <Util/Timer.h>
#include <Util/Utils.h>
#include <cstdint>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace MinSG {
namespace Profiling {

struct Profiler::Implementation {
	//! Timer used for time measurements
	Util::Timer timer;
	
	//! List of loggers
	std::unordered_set<Logger *> loggers;
};

Profiler::Profiler() : impl(new Implementation) {
}

Profiler::~Profiler() = default;

Action Profiler::createAction(const std::string & description) const {
	Util::GenericAttributeMap action;
	action.setValue(ATTR_description, Util::GenericAttribute::create(description));
	return action;
}

void Profiler::logAction(const Action & action) const {
	for(auto & logger : impl->loggers) {
		logger->log(action);
	}
}

void Profiler::annotateTime(Action & action, const Util::StringIdentifier & attribute) const {
	const auto currentTime = impl->timer.getNanoseconds();
	action.setValue(attribute, Util::GenericAttribute::create(currentTime));
}

void Profiler::annotateMemory(Action & action, const Util::StringIdentifier & attribute) const {
	const auto currentMemory = Util::Utils::getResidentSetMemorySize();
	action.setValue(attribute, Util::GenericAttribute::create(currentMemory));
}

Action Profiler::beginTimeMemoryAction(const std::string & description) const {
	auto action = createAction(description);
	annotateMemory(action, ATTR_memoryBegin);
	annotateTime(action, ATTR_timeBegin);
	return action;
}

void Profiler::endTimeMemoryAction(Action & action) const {
	annotateTime(action, ATTR_timeEnd);
	annotateMemory(action, ATTR_memoryEnd);
	logAction(action);
}

void Profiler::registerLogger(Logger * logger) {
	impl->loggers.insert(logger);
}

void Profiler::unregisterLogger(Logger * logger) {
	impl->loggers.erase(logger);
}

}
}
