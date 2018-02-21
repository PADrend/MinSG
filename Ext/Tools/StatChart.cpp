/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "StatChart.h"
#include <Util/Graphics/PixelAccessor.h>

namespace MinSG {

StatChart::StatChart(uint32_t width, uint32_t height, float _timeRange/*=50.0*/) :
	bitmap(new Util::Bitmap(width, height, Util::PixelFormat::RGBA)), timeRange(_timeRange), dataRows(Statistics::MAX_NUM_EVENT_TYPES) {

	setDescription(Statistics::EVENT_TYPE_GEOMETRY, "Geometry (#polygons)");
	setColor(Statistics::EVENT_TYPE_GEOMETRY, Util::Color4ub(0, 0, 0xff, 0xa0));
	setRange(Statistics::EVENT_TYPE_GEOMETRY, 200000);

	setDescription(Statistics::EVENT_TYPE_IDLE, "CPU idle");
	setColor(Statistics::EVENT_TYPE_IDLE, Util::Color4ub(0x80, 0x80, 0x80, 0x80));
	setRange(Statistics::EVENT_TYPE_IDLE, 200000);

	setDescription(Statistics::EVENT_TYPE_START_TEST, "OccTest start");
	setColor(Statistics::EVENT_TYPE_START_TEST, Util::Color4ub(0xFF, 0xFF, 0x80, 0x80));
	setRange(Statistics::EVENT_TYPE_START_TEST, 2);

	setDescription(Statistics::EVENT_TYPE_END_TEST_VISIBLE, "OccTest visible");
	setColor(Statistics::EVENT_TYPE_END_TEST_VISIBLE, Util::Color4ub(0x80, 0xFF, 0x80, 0x80));
	setRange(Statistics::EVENT_TYPE_END_TEST_VISIBLE, 2);

	setDescription(Statistics::EVENT_TYPE_END_TEST_INVISIBLE, "OccTest invisible");
	setColor(Statistics::EVENT_TYPE_END_TEST_INVISIBLE, Util::Color4ub(0xFF, 0x80, 0x80, 0x80));
	setRange(Statistics::EVENT_TYPE_END_TEST_INVISIBLE, 2);

	setDescription(Statistics::EVENT_TYPE_FRAME_END, "Frame end");
	setColor(Statistics::EVENT_TYPE_FRAME_END, Util::Color4ub(0xFF, 0x60, 0x60, 0x80));
	setRange(Statistics::EVENT_TYPE_FRAME_END, 1);


	// FIXME: BE: Actually this does not belong here, but I do not know where to put it otherwise.
	static const Statistics::eventType_t EVENT_TYPE_OUTOFCORE_BEGIN = 8;
	static const Statistics::eventType_t EVENT_TYPE_OUTOFCORE_END = 9;

	setDescription(EVENT_TYPE_OUTOFCORE_BEGIN, "OutOfCore: Begin work");
	setColor(EVENT_TYPE_OUTOFCORE_BEGIN, Util::Color4ub(255, 128, 0, 255));
	setRange(EVENT_TYPE_OUTOFCORE_BEGIN, 2.0f);

	setDescription(EVENT_TYPE_OUTOFCORE_END, "OutOfCore: End work");
	setColor(EVENT_TYPE_OUTOFCORE_END, Util::Color4ub(255, 128, 0, 255));
	setRange(EVENT_TYPE_OUTOFCORE_END, 1.0f);
}

StatChart::~StatChart() = default;

uint32_t StatChart::getWidth() const {
	return bitmap->getWidth();
}
uint32_t StatChart::getHeight() const {
	return bitmap->getHeight();
}

void StatChart::update(const Statistics & fStats) {
	Util::Reference<Util::PixelAccessor> pixels = Util::PixelAccessor::create(bitmap.get());
	if(pixels == nullptr)
		return;

	pixels->fill(0, 0, bitmap->getWidth(), bitmap->getHeight(), Util::Color4f(0, 0, 0, 0));

	// show grids
	static const Util::Color4ub gridColor(0xa0, 0xa0, 0xa0, 0xa0);
	for(float f = 10.0; f < timeRange; f += 10.0) {
		const uint32_t x = static_cast<uint32_t> (bitmap->getWidth() * f / timeRange);
		for (uint32_t row = 0; row < bitmap->getHeight(); ++row) {
			pixels->writeColor(x, row, gridColor);
		}
	}

	const float timeScale = getWidth() / (timeRange * 1000.0);
	for(size_t i = 0; i < fStats.getNumEvents(); ++i) {
		const Statistics::Event & event = fStats.getEvent(i);
		const int x = static_cast<int> (event.time * timeScale);
		if(x >= static_cast<int> (getWidth()))
			continue;

		const DataRow & dataRow = dataRows[event.type];
		const int yTo = std::max(static_cast<int> (bitmap->getHeight()) - static_cast<int> (event.value * dataRow.scale), 0);
		for(int y = bitmap->getHeight() - 1; y > yTo; --y)
			pixels->writeColor(x, y, dataRow.color);
	}

}

}
