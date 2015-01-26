/*
	This file is part of the MinSG library.
	Copyright (C) 2010-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "EventHandler.h"

namespace MinSG {

/**
 * [ctor]
 */
EventHandler::EventHandler() : mutex() {
}
/**
 * [dtor]
 */
EventHandler::~EventHandler() = default;
/**
 *
 */
bool EventHandler::process(const Util::UI::Event & e) {
	std::lock_guard<std::mutex> lock(mutex);
	return (processJoyEvent(e) || processMouseEvent(e) || processKeyEvent(e));
}
/**
 *
 */
void EventHandler::execute() {
	std::lock_guard<std::mutex> lock(mutex);
	doExecute();
}

}
