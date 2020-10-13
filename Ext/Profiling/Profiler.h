/*
	This file is part of the MinSG library extension Profiling.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_PROFILING_PROFILER_H_
#define MINSG_PROFILING_PROFILER_H_

#include "Action.h"
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>

namespace Util {
class GenericAttributeMap;
class StringIdentifier;
}
namespace MinSG {
//! @ingroup ext
namespace Profiling {
typedef Util::GenericAttributeMap Action;
class Logger;

/**
 * @brief Profiling code (measure running time, memory consumption and log it)
 * 
 * The profiler manages actions. An action is an extendable object that can be
 * annotated by arbitrary attributes. The profile contains convenience
 * functions to create actions and to annotate some standard attributes.
 * Different loggers can be attached to the profiler that are used to write
 * the actions to streams (e.g. a file, stdout). For example, the profiler can
 * be used to measure the duration and memory consumption of a code section:
 * @code
 * MinSG::Profiling::Profiler profiler;
 * MinSG::Profiling::LoggerPlainText logger(std::cout);
 * 
 * profiler.registerLogger(&logger);
 * 
 * auto prepareAction = profiler.beginTimeMemoryAction("My preparation code");
 * ...
 * My preparation code
 * ...
 * profiler.endTimeMemoryAction(prepareAction);
 * auto executeAction = profiler.beginTimeMemoryAction("My execute code");
 * ...
 * My execute code
 * ...
 * profiler.endTimeMemoryAction(executeAction);
 * 
 * profiler.unregisterLogger(&logger);
 * @endcode
 */
class Profiler {
	private:
		// Use Pimpl idiom
		struct Implementation;
		std::unique_ptr<Implementation> impl;

	public:
		MINSGAPI Profiler();
		MINSGAPI ~Profiler();

		/**
		 * Create a new action and set its description.
		 * 
		 * @param description Human-readable description of the action
		 * @return The created action
		 */
		MINSGAPI Action createAction(const std::string & description) const;

		/**
		 * Output an action to the associated loggers.
		 * 
		 * @param action Action that will be logged
		 */
		MINSGAPI void logAction(const Action & action) const;

		/**
		 * Measure the current time and store it in an attribute of an action.
		 * 
		 * @param action Action that will be annotated
		 * @param attribute Attribute name that will be added to the action
		 */
		MINSGAPI void annotateTime(Action & action,
						  const Util::StringIdentifier & attribute) const;

		/**
		 * Measure the current memory consumption and store it in an attribute
		 * of an action.
		 * 
		 * @param action Action that will be annotated
		 * @param attribute Attribute name that will be added to the action
		 */
		MINSGAPI void annotateMemory(Action & action,
							const Util::StringIdentifier & attribute) const;

		/**
		 * Create a new action, set its description, and store current time
		 * and memory consumption of the process.
		 * 
		 * @param description Human-readable description of the action
		 * @return The created action containing time and memory
		 */
		MINSGAPI Action beginTimeMemoryAction(const std::string & description) const;

		/**
		 * Finish an action. The current time and memory consumption will be
		 * stored to allow comparison with the beginning of the action. The
		 * action will be logged.
		 * 
		 * @param action Action that will be annotated and logged
		 */
		MINSGAPI void endTimeMemoryAction(Action & action) const;

		/**
		 * Register a logger that will be used to output profiling information.
		 * The information will be formatted internally by the logger.
		 * 
		 * @param output Logger that will be used to write profiling output.
		 * The given logger has to stay writeable at least until this object
		 * is destroyed or the logger is removed with @a unregisterLogger().
		 */
		MINSGAPI void registerLogger(Logger * output);

		/**
		 * Remove the registered logger.
		 * 
		 * @param output Logger that will be removed
		 */
		MINSGAPI void unregisterLogger(Logger * output);
};

}
}

#endif /* MINSG_PROFILING_PROFILER_H_ */
