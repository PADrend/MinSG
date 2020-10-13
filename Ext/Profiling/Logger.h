/*
	This file is part of the MinSG library extension Profiling.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_PROFILING_LOGGER_H_
#define MINSG_PROFILING_LOGGER_H_

#include "Action.h"
#include <iosfwd>
#include <memory>
#include <string>

namespace Util {
class StringIdentifier;
}
namespace MinSG {
namespace Profiling {

/**
 * Base class for logging actions. The format of the output that is written to
 * the associated stream is determined by selecting an appropriate subclass.
 */
class Logger {
	protected:
		std::ostream & output;
	public:
		MINSGAPI Logger(std::ostream & outputStream);
		MINSGAPI virtual ~Logger();

		//! Create formatted output for the given action
		virtual void log(const Action & action) = 0;
};

//! Logger for JSON formatted data
class LoggerJSON : public Logger {
	public:
		MINSGAPI LoggerJSON(std::ostream & outputStream);
		MINSGAPI virtual ~LoggerJSON();

		MINSGAPI void log(const Action & action) override;
};

//! Logger for human-readable plain text data
class LoggerPlainText : public Logger {
	public:
		MINSGAPI LoggerPlainText(std::ostream & outputStream);
		MINSGAPI virtual ~LoggerPlainText();

		MINSGAPI void log(const Action & action) override;
};

//! Logger for tab-separated values
class LoggerTSV : public Logger {
	private:
		// Use Pimpl idiom
		struct Implementation;
		std::unique_ptr<Implementation> impl;

	public:
		/**
		 * Create a new logger for tab-separated values. All entries in the
		 * output file will be formatted by means of a predefined set of
		 * columns. Before logging any actions, the default set can be
		 * extended (see addColumn()). At the beginning, the default columns
		 * are ATTR_description, ATTR_memoryBegin ATTR_memoryEnd,
		 * ATTR_timeBegin, and ATTR_timeEnd.
		 */
		MINSGAPI LoggerTSV(std::ostream & outputStream);
		MINSGAPI virtual ~LoggerTSV();

		/**
		 * Add a column to the set of columns used for formatting the actions
		 * that are logged.
		 * 
		 * @param columnName String identifier that is used in actions later on
		 */
		MINSGAPI void addColumn(const Util::StringIdentifier & columnName);

		MINSGAPI void log(const Action & action) override;
};

//! Logger for XML formatted data
class LoggerXML : public Logger {
	public:
		MINSGAPI LoggerXML(std::ostream & outputStream);
		MINSGAPI virtual ~LoggerXML();

		MINSGAPI void log(const Action & action) override;
};

}
}

#endif /* MINSG_PROFILING_LOGGER_H_ */
