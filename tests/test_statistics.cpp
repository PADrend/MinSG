/*
	This file is part of the MinSG library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include <MinSG/Core/Statistics.h>
#include <MinSG/Ext/Profiling/Profiler.h>
#include <Util/GenericAttribute.h>
#include <Util/StringIdentifier.h>
#include <Util/Timer.h>
#include <cstdint>
#include <fstream>

// Prevent warning
int test_statistics();

void testStatistics(uint32_t numEventTypes, uint32_t numEventsOverall) {
	MinSG::Statistics stats;
	stats.enableEvents();

	while(stats.getNumEvents() < numEventsOverall) {
		for(uint32_t eventType = 0; eventType < numEventTypes; ++eventType) {
			stats.pushEvent(eventType, 42.0);
		}
	}
}

void testProfiler(uint32_t numEventTypes, uint32_t numEventsOverall) {
	static Util::StringIdentifier ATTR_time("time");
	static Util::StringIdentifier ATTR_testValue("testValue");
	MinSG::Profiling::Profiler profiler;
	uint32_t eventCount = 0;
	while(eventCount < numEventsOverall) {
		for(uint32_t eventType = 0; eventType < numEventTypes; ++eventType) {
			auto action = profiler.createAction("Test");
			profiler.annotateTime(action, ATTR_time);
			action.setValue(ATTR_testValue, Util::GenericAttribute::create(42.0));
			profiler.logAction(action);
			++eventCount;
		}
	}
}

int test_statistics() {
	std::ofstream output("test_statistics.tsv");
	output << "class\tnumEventTypes\tnumEventsOverall\tduration\n";
	Util::Timer timer;
	for(uint32_t numRuns = 1; numRuns < 5; ++numRuns) {
		for(uint32_t numEventsOverall = 1; numEventsOverall <= 1000000; numEventsOverall *= 10) {
			for(uint32_t numEventTypes = 1; numEventTypes <= 32; numEventTypes *= 2) {
				timer.reset();
				testStatistics(numEventTypes, numEventsOverall);
				timer.stop();
				output << "Statistics\t" << numEventTypes << '\t' << numEventsOverall << '\t' << timer.getNanoseconds() << '\n';
				timer.reset();
				testProfiler(numEventTypes, numEventsOverall);
				timer.stop();
				output << "Profiler\t" << numEventTypes << '\t' << numEventsOverall << '\t' << timer.getNanoseconds() << '\n';
			}
		}
	}
	return EXIT_SUCCESS;
}
