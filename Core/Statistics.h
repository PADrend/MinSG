/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_STATISTICS_H
#define MINSG_STATISTICS_H

#include <cstdint>
#include <string>
#include <vector>
#include <Util/Timer.h>
#include <utility>

namespace Rendering {
class Mesh;
}
namespace MinSG {
class Node;

/**
 *  [Statistics]
 */
class Statistics {

	//!	@name General
	//	@{
	public:
		MINSGAPI Statistics();

		MINSGAPI void beginFrame(int32_t framNumber=-1);
		MINSGAPI void endFrame();

		uint32_t getFrameNumberCounter() const {
			return frameNumberCounter;
		}
		uint32_t getFrameDurationCounter() const {
			return frameDurationCounter;
		}
		uint32_t getIORateReadCounter() const {
			return ioRateReadCounter;
		}
		uint32_t getIORateWriteCounter() const {
			return ioRateWriteCounter;
		}
		uint32_t getVBOCounter() const {
			return vboCounter;
		}
		uint32_t getTrianglesCounter() const {
			return trianglesCounter;
		}
		uint32_t getLinesCounter() const {
			return linesCounter;
		}
		uint32_t getPointsCounter() const {
			return pointsCounter;
		}
		uint32_t getNodeCounter() const {
			return nodeCounter;
		}

	private:
		//! Key of frame number counter
		uint32_t frameNumberCounter;
		//! Key of frame duration counter
		uint32_t frameDurationCounter;
		//! Key of read IO rate counter
		uint32_t ioRateReadCounter;
		//! Key of wrtie IO rate counter
		uint32_t ioRateWriteCounter;
		//! Key of VBO counter
		uint32_t vboCounter;
		//! Key of triangles counter
		uint32_t trianglesCounter;
		//! Key of lines counter
		uint32_t linesCounter;
		//! Key of points counter
		uint32_t pointsCounter;
		//! Key of node counter
		uint32_t nodeCounter;
	//	@}

	// ------------------------------------------------------------

	//!	@name Counters
	//	@{
	public:
		MINSGAPI static const uint32_t COUNTER_KEY_INVALID;

		int32_t getValueAsInt(uint32_t key)const			{	return static_cast<int32_t>(counters[key].value);	}
		double getValueAsDouble(uint32_t key)const 		{	return counters[key].value;	}


		void addValue(uint32_t key,int value) 				{	addValue(key, static_cast<double>(value));	}
		void addValue(uint32_t key,unsigned int value) 		{	addValue(key, static_cast<double>(value));	}
		void addValue(uint32_t key,double value){
			counters[key].value += value;
		}

		void setValue(uint32_t key,int value) 			{	setValue(key, static_cast<double>(value));	}
		void setValue(uint32_t key,unsigned int value) 	{	setValue(key, static_cast<double>(value));	}
		void setValue(uint32_t key,double value){
			counters[key].value = value;
		}
		void unsetValue(uint32_t key){
			counters[key].value = 0.0;
		}

		const std::string & getDescription(uint32_t key) const {
			return counters[key].description;
		}
		const std::string & getUnit(uint32_t key) const {
			return counters[key].unit;
		}
		/**
		 * Create a new counter with the given description and unit.
		 *
		 * @param description A short description of the value counted
		 * @param unit Unit of the value counted
		 * @return Key that is used to access the counter
		 */
		MINSGAPI uint32_t addCounter(const std::string & description, const std::string & unit);

		//! Return the number of counters. This value can be used in a loop iterating over the counters.
		std::size_t getNumCounters() const {
			return counters.size();
		}

		/**
		 * Search all counters and return the counter that has the same
		 * description as the requested one.
		 * 
		 * @param description Requested description
		 * @return Key that is used to access the counter. If there is no
		 * counter with the requested description, @a COUNTER_KEY_INVALID is
		 * returned.
		 */
		MINSGAPI uint32_t getCounterForDescription(const std::string & description) const;

		MINSGAPI void countMesh(const Rendering::Mesh & mesh, uint32_t primitiveCount);

		MINSGAPI void countNode(const Node * node);

	private:
		struct Counter {
			std::string description;
			std::string unit;
			double value;

			Counter(std::string _description, std::string _unit) :
				description(std::move(_description)), unit(std::move(_unit)), value(0.0) {
			}
		};
		std::vector<Counter> counters;

		Util::Timer frameTimer;
	//	@}

	// ------------------------------------------------------------

	//!	@name Events
	//	@{
	public:
		typedef uint8_t eventType_t;

		// this is no enum, because it should be extendible from outside of the class.
		static const eventType_t EVENT_TYPE_GEOMETRY = 1;
		static const eventType_t EVENT_TYPE_IDLE = 2;
		static const eventType_t EVENT_TYPE_START_TEST = 3;
		static const eventType_t EVENT_TYPE_END_TEST_VISIBLE = 5;
		static const eventType_t EVENT_TYPE_END_TEST_INVISIBLE = 6;
		static const eventType_t EVENT_TYPE_FRAME_END = 7;

		//! @note The constants EVENT_TYPE_... with values {8, 9} are used by MinSG::OutOfCore.

		static const unsigned int MAX_NUM_EVENT_TYPES = 10;

		struct Event{
			eventType_t type;
			double time;
			double value;
			Event(eventType_t _type,double _time, double _value) : type(_type),time(_time),value(_value) {	}
			Event(const Event & e) = default;
			Event(Event && e) = default;
			Event & operator=(const Event & e) = default;
			Event & operator=(Event && e) = default;
			bool operator==(const Event & e)const{
				return type==e.type && time==e.time && value==e.value;
			}

		};
		bool areEventsEnabled()const				{	return eventsEnabled;	}
		void enableEvents()							{	eventsEnabled=true;	}
		void disableEvents()						{	eventsEnabled=false;	}

		MINSGAPI void pushEvent(eventType_t type,double value);
		const Event & getEvent(size_t index) const {
			return events[index];
		}
		//! Return the number of events. This value can be used in a loop iterating over the events.
		std::size_t getNumEvents() const {
			return events.size();
		}


	private:
		bool eventsEnabled;
		std::vector<Event> events;
	//	@}

};
}
#endif // MINSG_STATISTICS_H
