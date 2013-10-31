/*
	This file is part of the MinSG library extension Profiling.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "Logger.h"
#include "Action.h"
#include <Util/GenericAttribute.h>
#include <Util/StringIdentifier.h>
#include <ostream>
#include <vector>

namespace MinSG {
namespace Profiling {

Logger::Logger(std::ostream & outputStream) :
	output(outputStream) {
}
Logger::~Logger() = default;


LoggerJSON::LoggerJSON(std::ostream & outputStream) :
	Logger(outputStream) {
}
LoggerJSON::~LoggerJSON() = default;
void LoggerJSON::log(const Action & action) {
	output << action.toJSON();
}


LoggerPlainText::LoggerPlainText(std::ostream & outputStream) :
	Logger(outputStream) {

}
LoggerPlainText::~LoggerPlainText() = default;
void LoggerPlainText::log(const Action & action) {
	output << action.toString();
}


struct LoggerTSV::Implementation {
	std::vector<Util::StringIdentifier> columns;
	bool isHeaderWritten;
	
	Implementation() : columns(), isHeaderWritten(false) {
	}
};
LoggerTSV::LoggerTSV(std::ostream & outputStream) :
	Logger(outputStream), impl(new Implementation) {
	// ATTR_description is hard-coded to simplify loops
	impl->columns.push_back(ATTR_memoryBegin);
	impl->columns.push_back(ATTR_memoryEnd);
	impl->columns.push_back(ATTR_timeBegin);
	impl->columns.push_back(ATTR_timeEnd);
}
LoggerTSV::~LoggerTSV() = default;
void LoggerTSV::addColumn(const Util::StringIdentifier & columnName) {
	impl->columns.push_back(columnName);
}
void LoggerTSV::log(const Action & action) {
	if(!impl->isHeaderWritten) {
		impl->isHeaderWritten = true;
		output << ATTR_description.toString();
		for(const auto & column : impl->columns) {
			output << '\t' << column.toString();
		}
		output << '\n';
	}

	output << action.getValue(ATTR_description)->toString();
	for(const auto & column : impl->columns) {
		output << '\t';
		const auto * value = action.getValue(column);
		if(value != nullptr) {
			output << value->toString();
		}
	}
	output << '\n';
}


LoggerXML::LoggerXML(std::ostream & outputStream) :
	Logger(outputStream) {
	output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	output << "<profiling>\n";
}
LoggerXML::~LoggerXML() {
	output << "</profiling>\n";
}
void LoggerXML::log(const Action & action) {
	output << "\t<action>\n";
	for(const auto & keyValuePair : action) {
		const auto tagName = keyValuePair.first.toString();
		output << "\t\t<" << tagName << '>' << 
						keyValuePair.second->toString() << 
						"</" << tagName << ">\n";
	}
	output << "\t</action>\n";
}

}
}
