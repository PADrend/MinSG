/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "Statistics.h"
#include <Rendering/Mesh/Mesh.h>
#include <Util/Utils.h>
#include <limits>

namespace MinSG {

const uint32_t Statistics::COUNTER_KEY_INVALID = std::numeric_limits<uint32_t>::max();

Statistics::Statistics() : counters(), eventsEnabled(false) {
	frameNumberCounter = addCounter("frame number", "1");
	frameDurationCounter = addCounter("frame duration", "ms");
	vboCounter = addCounter("VBOs rendered", "1");
	trianglesCounter = addCounter("triangles rendered", "1");
	linesCounter = addCounter("lines rendered", "1");
	pointsCounter = addCounter("points rendered", "1");
	nodeCounter = addCounter("geometry nodes rendered", "1");

	ioRateReadCounter = addCounter("I/O rate read", "MiB/s");
	ioRateWriteCounter = addCounter("I/O rate write", "MiB/s");
}

void Statistics::beginFrame(int32_t newFrameNumber/*=-1*/){
	if(eventsEnabled) {
		events.clear();
	}
	const int32_t oldFrameNumber = getValueAsInt(frameNumberCounter);
	if(newFrameNumber == -1) {
		newFrameNumber = oldFrameNumber + 1;
	}
	if(oldFrameNumber != newFrameNumber) {
		for(uint_fast32_t i = 0; i < counters.size(); ++i) {
			// Do not reset the IO counters.
			 if(i != ioRateReadCounter && i != ioRateWriteCounter) {
				 unsetValue(i);
			 }
		}
		setValue(frameNumberCounter, newFrameNumber);
	}
	frameTimer.reset();
}

void Statistics::endFrame() {
	setValue(frameDurationCounter, frameTimer.getMilliseconds());

	{ // IO
		static Util::Timer ioTimer;
		static size_t lastBytesRead = 0;
		static size_t lastBytesWritten = 0;

		const uint64_t duration = ioTimer.getNanoseconds();
		// Update every second.
		if(duration > 1000000000) {
			const size_t bytesRead = Util::Utils::getIOBytesRead();
			const size_t bytesWritten = Util::Utils::getIOBytesWritten();

			const double MebiPerSecond = 1024.0 * 1024.0 * 1.0e-9 * static_cast<double>(duration);
			const double readRate = static_cast<double>(bytesRead - lastBytesRead) / MebiPerSecond;
			const double writeRate = static_cast<double>(bytesWritten - lastBytesWritten) / MebiPerSecond;

			ioTimer.reset();
			lastBytesRead = bytesRead;
			lastBytesWritten = bytesWritten;

			setValue(ioRateReadCounter, readRate);
			setValue(ioRateWriteCounter, writeRate);
		}
	}

	pushEvent(EVENT_TYPE_FRAME_END, 1);
}

uint32_t Statistics::getCounterForDescription(const std::string & description) const {
	for(uint_fast32_t i = 0; i < counters.size(); ++i) {
		if(counters[i].description == description) {
			return i;
		}
	}
	return COUNTER_KEY_INVALID;
}

void Statistics::countMesh(const Rendering::Mesh & mesh, uint32_t primitiveCount) {
	// TODO: Check if the mesh is stored in VBOs
	addValue(vboCounter, 1);

	switch(mesh.getDrawMode()) {
		case Rendering::Mesh::DRAW_POINTS:
			addValue(pointsCounter, primitiveCount);
			break;
		case Rendering::Mesh::DRAW_LINE_STRIP:
		case Rendering::Mesh::DRAW_LINE_LOOP:
		case Rendering::Mesh::DRAW_LINES:
			addValue(linesCounter, primitiveCount);
			break;
		case Rendering::Mesh::DRAW_TRIANGLES:
			addValue(trianglesCounter, primitiveCount);
			break;
		default:
			break;
	}
}

void Statistics::countNode(const Node * node) {
	if(node == nullptr) {
		return;
	}

	addValue(nodeCounter, 1);
}

uint32_t Statistics::addCounter(const std::string & description, const std::string & unit) {
	const uint32_t newKey = static_cast<uint32_t>(counters.size());
	counters.emplace_back(description, unit);
	return newKey;
}

void Statistics::pushEvent(eventType_t type, double value) {
	if(eventsEnabled) {
		events.emplace_back(type, frameTimer.getMicroseconds(), value);
	}
}

}
