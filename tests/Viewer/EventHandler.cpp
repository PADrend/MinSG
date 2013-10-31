/*
	This file is part of the MinSG library.
	Copyright (C) 2010-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "EventHandler.h"
#include <Util/Concurrency/Concurrency.h>
#include <Util/Concurrency/Mutex.h>
using namespace MinSG;

/**
 * [ctor]
 */
EventHandler::EventHandler() {
	lock = Util::Concurrency::createMutex();
}
/**
 * [dtor]
 */
EventHandler::~EventHandler() {
	delete lock;
}
/**
 *
 */
bool EventHandler::process(const Util::UI::Event & e) {
	lock->lock();
	if (processJoyEvent(e) || processMouseEvent(e) || processKeyEvent(e)) {
		lock->unlock();
		return true;
	}
	lock->unlock();
	return false;
}
/**
 *
 */
void EventHandler::execute() {
	lock->lock();
	doExecute();
	lock->unlock();
}
