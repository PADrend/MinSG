/*
	This file is part of the MinSG library.
	Copyright (C) 2010-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "MoveNodeHandler.h"
#include <Util/UI/Event.h>

using namespace MinSG;

MoveNodeHandler::MoveNodeHandler():EventHandler(), joystickId(0) {
	wrapper = map<unsigned int, Wrapper*>();
}

MoveNodeHandler::~MoveNodeHandler() {
	while (!wrapper.empty()) {
		Wrapper *w = wrapper.begin()->second;
		wrapper.erase(wrapper.begin());
		delete w;
	}
}


void MoveNodeHandler::setAction(const int trigger, Wrapper *w) {
	map<unsigned int, Wrapper*>::iterator iter;
	iter = wrapper.find(trigger);
	if (iter != wrapper.end())
		wrapper.erase(iter);
	if (w)
		wrapper[trigger] = w;
}

void MoveNodeHandler::setJoystick(const int id) {
	joystickId = id;
}

void MoveNodeHandler::setSpeedFactor(const int action , const float s) {
	map<unsigned, Wrapper*>::iterator iter;
	for (iter = wrapper.begin(); iter != wrapper.end(); iter++) {
		iter->second->setSpeedFactor(action, s);
	}
}

void MoveNodeHandler::incSpeedFactor(const int action, const float factor) {
	map<unsigned, Wrapper*>::iterator iter;
	for (iter = wrapper.begin(); iter != wrapper.end(); iter++) {
		iter->second->incSpeedFactor(action, factor);
	}
}

bool MoveNodeHandler::processMouseEvent(const Util::UI::Event & e) {
	map<unsigned int, Wrapper*>::iterator iter;
	switch (e.type) {
		case Util::UI::EVENT_MOUSE_MOTION: {
				bool ret = false;
				iter = wrapper.find(MINSG_MOUSE_MOTION_X);
				if (iter != wrapper.end()) {
					iter->second->setSpeed(e.motion.deltaX);
					ret = true;
				}
				iter = wrapper.find(MINSG_MOUSE_MOTION_Y);
				if (iter != wrapper.end()) {
					iter->second->setSpeed(e.motion.deltaY);
					ret = true;
				}
				return ret;
			}
		case Util::UI::EVENT_MOUSE_BUTTON:
			switch (e.button.button) {
				case Util::UI::MOUSE_BUTTON_LEFT:
					iter = wrapper.find(MINSG_MOUSE_BUTTON_LEFT);
					if (iter != wrapper.end()) {
						iter->second->setSpeed(e.button.pressed ? 1 : 0);
						return true;
					}
					break;
				case Util::UI::MOUSE_BUTTON_MIDDLE:
					iter = wrapper.find(MINSG_MOUSE_BUTTON_MIDDLE);
					if (iter != wrapper.end()) {
						iter->second->setSpeed(e.button.pressed ? 1 : 0);
						return true;
					}
					break;
				case Util::UI::MOUSE_BUTTON_RIGHT:
					iter = wrapper.find(MINSG_MOUSE_BUTTON_RIGHT);
					if (iter != wrapper.end()) {
						iter->second->setSpeed(e.button.pressed ? 1 : 0);
						return true;
					}
					break;
				case Util::UI::MOUSE_WHEEL_UP:
					iter = wrapper.find(MINSG_MOUSE_BUTTON_WHEELUP);
					if (iter != wrapper.end()) {
						iter->second->setSpeed(e.button.pressed ? 1 : 0);
						return true;
					}
					break;
				case Util::UI::MOUSE_WHEEL_DOWN:
					iter = wrapper.find(MINSG_MOUSE_BUTTON_WHEELDOWN);
					if (iter != wrapper.end()) {
						iter->second->setSpeed(e.button.pressed ? 1 : 0);
						return true;
					}
				default:
					return false;
			}
			break;

		default:
			return false;
	}
	return false;
}

bool MoveNodeHandler::processJoyEvent(const Util::UI::Event & e) {
	map<unsigned int, Wrapper*>::iterator iter;
	switch (e.type) {
		case Util::UI::EVENT_JOY_AXIS:
			if (e.joyAxis.joystick != joystickId) return false;
			iter = wrapper.find(MINSG_JOY_STICK_LEFT_X+e.joyAxis.axis);
			if (iter != wrapper.end()) {
				iter->second->setSpeed(e.joyAxis.value);
				return true;
			}
			break;
		case Util::UI::EVENT_JOY_BUTTON:
			if (e.joyButton.joystick != joystickId) return false;
			iter = wrapper.find(MINSG_JOY_BUTTON_1+e.joyButton.button);
			if (iter != wrapper.end()) {
				iter->second->setSpeed(e.joyButton.pressed ? 1 : 0);
				return true;
			}
			break;
		case Util::UI::EVENT_JOY_HAT: {
				bool ret = false;
				if (e.joyHat.joystick != joystickId) return false;
				iter = wrapper.find(MINSG_JOY_HAT_DOWN);
				if (iter != wrapper.end()) {
					iter->second->setSpeed(e.joyHat.value & Util::UI::MASK_HAT_DOWN ? 1 : 0);
					ret = true;
				}
				iter = wrapper.find(MINSG_JOY_HAT_UP);
				if (iter != wrapper.end()) {
					iter->second->setSpeed(e.joyHat.value & Util::UI::MASK_HAT_UP ? 1 : 0);
					ret = true;
				}
				iter = wrapper.find(MINSG_JOY_HAT_LEFT);
				if (iter != wrapper.end()) {
					iter->second->setSpeed(e.joyHat.value & Util::UI::MASK_HAT_LEFT ? 1 : 0);
					ret = true;
				}
				iter = wrapper.find(MINSG_JOY_HAT_RIGHT);
				if (iter != wrapper.end()) {
					iter->second->setSpeed(e.joyHat.value & Util::UI::MASK_HAT_RIGHT ? 1 : 0);
					ret = true;
				}
				return ret;
			}
			break;
		default:
			return false;
	}
	return false;
}

bool MoveNodeHandler::processKeyEvent(const Util::UI::Event & e) {
	map<unsigned int, Wrapper*>::iterator iter;
	iter = wrapper.find(e.keyboard.key);
	if (iter != wrapper.end()) {
		iter->second->setSpeed((e.keyboard.pressed ? 1 : 0));
		iter++;
		return true;
	}
	return false;
}

void MoveNodeHandler::doExecute() {
	for (map<unsigned int, Wrapper*>::iterator iter = wrapper.begin(); iter != wrapper.end(); iter++) {
		iter->second->execute();
	}
}

void MoveNodeHandler::initRalf(MoveNodeHandler *eventHandler, Node *camera) {

	// key
	eventHandler->setAction('w', new KeyWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('s', new KeyWrapper(camera, MINSG_MOVE|MINSG_POSITIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('a', new KeyWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('d', new KeyWrapper(camera, MINSG_MOVE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('f', new KeyWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_Y|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('r', new KeyWrapper(camera, MINSG_MOVE|MINSG_POSITIV_Y|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('e', new KeyWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('q', new KeyWrapper(camera, MINSG_ROTATE|MINSG_POSITIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(Util::UI::KEY_DOWN, new KeyWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(Util::UI::KEY_UP, new KeyWrapper(camera, MINSG_ROTATE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(Util::UI::KEY_RIGHT, new KeyWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_Y|MINSG_COORDSYS_RELATIVE));
	eventHandler->setAction(Util::UI::KEY_LEFT, new KeyWrapper(camera, MINSG_ROTATE|MINSG_POSITIV_Y|MINSG_COORDSYS_RELATIVE));
	// mouse
	Wrapper *x = new MouseMotionWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_Y|MINSG_COORDSYS_RELATIVE);
	Wrapper *y = new MouseMotionWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_X|MINSG_COORDSYS_LOCAL);
	Wrapper *lbx = new MouseMotionWrapper(camera, MINSG_MOVE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL);
	Wrapper *lby = new MouseMotionWrapper(camera, MINSG_MOVE|MINSG_POSITIV_Z|MINSG_COORDSYS_LOCAL);
	Wrapper *rbx = new MouseMotionWrapper(camera, MINSG_MOVE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL);
	Wrapper *rby = new MouseMotionWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_Y|MINSG_COORDSYS_LOCAL);
	eventHandler->setAction(MINSG_MOUSE_BUTTON_MIDDLE, new ExchangeWrapper(eventHandler, x, y, lbx, lby, rbx, rby));
	// speedControl
	SpeedWrapper *splus = new SpeedWrapper(eventHandler, MINSG_MOVE, 1.5f);
	SpeedWrapper *sminus = new SpeedWrapper(eventHandler, MINSG_MOVE, 1/1.5f);
	eventHandler->setAction(MINSG_MOUSE_BUTTON_WHEELUP, splus);
	eventHandler->setAction(MINSG_MOUSE_BUTTON_WHEELDOWN, sminus);
	eventHandler->setAction(Util::UI::KEY_KPADD, splus);
	eventHandler->setAction(Util::UI::KEY_KPSUBTRACT, sminus);
	eventHandler->setAction(']', splus);
	eventHandler->setAction('/', sminus);
	// joystick / gamepad
	eventHandler->setAction(MINSG_JOY_BUTTON_6, splus);
	eventHandler->setAction(MINSG_JOY_BUTTON_8, sminus);
	eventHandler->setAction(MINSG_JOY_STICK_LEFT_X, new AnalogStickWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_Y|MINSG_COORDSYS_RELATIVE));
	eventHandler->setAction(MINSG_JOY_STICK_LEFT_Y, new AnalogStickWrapper(camera, MINSG_MOVE|MINSG_POSITIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_STICK_RIGHT_X, new AnalogStickWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_Y|MINSG_COORDSYS_RELATIVE));
	eventHandler->setAction(MINSG_JOY_STICK_RIGHT_Y, new AnalogStickWrapper(camera, MINSG_ROTATE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_HAT_DOWN, new ButtonWrapper(camera, MINSG_MOVE|MINSG_POSITIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_HAT_UP, new ButtonWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_HAT_RIGHT, new ButtonWrapper(camera, MINSG_MOVE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_HAT_LEFT, new ButtonWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_BUTTON_5, new ButtonWrapper(camera, MINSG_MOVE|MINSG_POSITIV_Y|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_BUTTON_7, new ButtonWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_Y|MINSG_COORDSYS_LOCAL));
	// speed
	eventHandler->setSpeedFactor(MINSG_MOVE, 10);
}

void MoveNodeHandler::initClaudius(MoveNodeHandler *eventHandler, Node *camera) {

	// key
	eventHandler->setAction('w', new KeyWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('s', new KeyWrapper(camera, MINSG_MOVE|MINSG_POSITIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('a', new KeyWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('d', new KeyWrapper(camera, MINSG_MOVE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('f', new KeyWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_Y|MINSG_COORDSYS_RELATIVE));
	eventHandler->setAction('r', new KeyWrapper(camera, MINSG_MOVE|MINSG_POSITIV_Y|MINSG_COORDSYS_RELATIVE));
	eventHandler->setAction('e', new KeyWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('q', new KeyWrapper(camera, MINSG_ROTATE|MINSG_POSITIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(Util::UI::KEY_UP, new KeyWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(Util::UI::KEY_DOWN, new KeyWrapper(camera, MINSG_ROTATE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(Util::UI::KEY_RIGHT, new KeyWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_Y|MINSG_COORDSYS_RELATIVE));
	eventHandler->setAction(Util::UI::KEY_LEFT, new KeyWrapper(camera, MINSG_ROTATE|MINSG_POSITIV_Y|MINSG_COORDSYS_RELATIVE));
	// mouse
	Wrapper *x = new MouseMotionWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_Y|MINSG_COORDSYS_RELATIVE);
	Wrapper *y = new MouseMotionWrapper(camera, MINSG_ROTATE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL);
	Wrapper *lbx = new MouseMotionWrapper(camera, MINSG_MOVE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL);
	Wrapper *lby = new MouseMotionWrapper(camera, MINSG_MOVE|MINSG_POSITIV_Z|MINSG_COORDSYS_LOCAL);
	Wrapper *rbx = new MouseMotionWrapper(camera, MINSG_MOVE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL);
	Wrapper *rby = new MouseMotionWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_Y|MINSG_COORDSYS_LOCAL);
	eventHandler->setAction(MINSG_MOUSE_BUTTON_MIDDLE, new ExchangeWrapper(eventHandler, x, y, lbx, lby, rbx, rby));
	// speedControl
	SpeedWrapper *splus = new SpeedWrapper(eventHandler, MINSG_MOVE, 1.5f);
	SpeedWrapper *sminus = new SpeedWrapper(eventHandler, MINSG_MOVE, 1/1.5f);
	eventHandler->setAction(MINSG_MOUSE_BUTTON_WHEELUP, splus);
	eventHandler->setAction(MINSG_MOUSE_BUTTON_WHEELDOWN, sminus);
	eventHandler->setAction(Util::UI::KEY_KPADD, splus);
	eventHandler->setAction(Util::UI::KEY_KPSUBTRACT, sminus);
	eventHandler->setAction(']', splus);
	eventHandler->setAction('/', sminus);
	// joystick / gamepad
	eventHandler->setAction(MINSG_JOY_BUTTON_6, splus);
	eventHandler->setAction(MINSG_JOY_BUTTON_8, sminus);
	eventHandler->setAction(MINSG_JOY_STICK_LEFT_X, new AnalogStickWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_Y|MINSG_COORDSYS_RELATIVE));
	eventHandler->setAction(MINSG_JOY_STICK_LEFT_Y, new AnalogStickWrapper(camera, MINSG_MOVE|MINSG_POSITIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_STICK_RIGHT_X, new AnalogStickWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_Y|MINSG_COORDSYS_RELATIVE));
	eventHandler->setAction(MINSG_JOY_STICK_RIGHT_Y, new AnalogStickWrapper(camera, MINSG_ROTATE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_HAT_DOWN, new ButtonWrapper(camera, MINSG_MOVE|MINSG_POSITIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_HAT_UP, new ButtonWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_HAT_RIGHT, new ButtonWrapper(camera, MINSG_MOVE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_HAT_LEFT, new ButtonWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_BUTTON_5, new ButtonWrapper(camera, MINSG_MOVE|MINSG_POSITIV_Y|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_BUTTON_7, new ButtonWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_Y|MINSG_COORDSYS_LOCAL));
	// speed
	eventHandler->setSpeedFactor(MINSG_MOVE, 10);
}

void MoveNodeHandler::initStephan(MoveNodeHandler *eventHandler, Node *camera) {

	// key
	eventHandler->setAction('w', new KeyWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('s', new KeyWrapper(camera, MINSG_MOVE|MINSG_POSITIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('a', new KeyWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('d', new KeyWrapper(camera, MINSG_MOVE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('f', new KeyWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_Y|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('r', new KeyWrapper(camera, MINSG_MOVE|MINSG_POSITIV_Y|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('e', new KeyWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('q', new KeyWrapper(camera, MINSG_ROTATE|MINSG_POSITIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(Util::UI::KEY_DOWN, new KeyWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(Util::UI::KEY_UP, new KeyWrapper(camera, MINSG_ROTATE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(Util::UI::KEY_RIGHT, new KeyWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_Y|MINSG_COORDSYS_RELATIVE));
	eventHandler->setAction(Util::UI::KEY_LEFT, new KeyWrapper(camera, MINSG_ROTATE|MINSG_POSITIV_Y|MINSG_COORDSYS_RELATIVE));
	// mouse
	Wrapper *x = new MouseMotionWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_Y|MINSG_COORDSYS_RELATIVE);
	Wrapper *y = new MouseMotionWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_X|MINSG_COORDSYS_LOCAL);
	Wrapper *lbx = new MouseMotionWrapper(camera, MINSG_MOVE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL);
	Wrapper *lby = new MouseMotionWrapper(camera, MINSG_MOVE|MINSG_POSITIV_Z|MINSG_COORDSYS_LOCAL);
	Wrapper *rbx = new MouseMotionWrapper(camera, MINSG_MOVE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL);
	Wrapper *rby = new MouseMotionWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_Y|MINSG_COORDSYS_LOCAL);
	eventHandler->setAction(MINSG_MOUSE_BUTTON_MIDDLE, new ExchangeWrapper(eventHandler, x, y, lbx, lby, rbx, rby));
	// speedControl
	SpeedWrapper *splus = new SpeedWrapper(eventHandler, MINSG_MOVE, 1.5f);
	SpeedWrapper *sminus = new SpeedWrapper(eventHandler, MINSG_MOVE, 1/1.5f);
	eventHandler->setAction(MINSG_MOUSE_BUTTON_WHEELUP, splus);
	eventHandler->setAction(MINSG_MOUSE_BUTTON_WHEELDOWN, sminus);
	eventHandler->setAction(Util::UI::KEY_KPADD, splus);
	eventHandler->setAction(Util::UI::KEY_KPSUBTRACT, sminus);
	eventHandler->setAction(']', splus);
	eventHandler->setAction('/', sminus);
	// joystick / gamepad
	eventHandler->setAction(MINSG_JOY_BUTTON_6, splus);
	eventHandler->setAction(MINSG_JOY_BUTTON_8, sminus);
	eventHandler->setAction(MINSG_JOY_STICK_LEFT_X, new AnalogStickWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_Y|MINSG_COORDSYS_RELATIVE));
	eventHandler->setAction(MINSG_JOY_STICK_LEFT_Y, new AnalogStickWrapper(camera, MINSG_MOVE|MINSG_POSITIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_STICK_RIGHT_X, new AnalogStickWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_Y|MINSG_COORDSYS_RELATIVE));
	eventHandler->setAction(MINSG_JOY_STICK_RIGHT_Y, new AnalogStickWrapper(camera, MINSG_ROTATE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_HAT_DOWN, new ButtonWrapper(camera, MINSG_MOVE|MINSG_POSITIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_HAT_UP, new ButtonWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_HAT_RIGHT, new ButtonWrapper(camera, MINSG_MOVE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_HAT_LEFT, new ButtonWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_BUTTON_5, new ButtonWrapper(camera, MINSG_MOVE|MINSG_POSITIV_Y|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_BUTTON_7, new ButtonWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_Y|MINSG_COORDSYS_LOCAL));
	// speed
	eventHandler->setSpeedFactor(MINSG_MOVE, 1);
}

void MoveNodeHandler::initAdrian(MoveNodeHandler *eventHandler, Node *camera) {

	// key
	eventHandler->setAction('w', new KeyWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('s', new KeyWrapper(camera, MINSG_MOVE|MINSG_POSITIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('a', new KeyWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('d', new KeyWrapper(camera, MINSG_MOVE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('f', new KeyWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_Y|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('r', new KeyWrapper(camera, MINSG_MOVE|MINSG_POSITIV_Y|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('e', new KeyWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction('q', new KeyWrapper(camera, MINSG_ROTATE|MINSG_POSITIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(Util::UI::KEY_DOWN, new KeyWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(Util::UI::KEY_UP, new KeyWrapper(camera, MINSG_ROTATE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(Util::UI::KEY_RIGHT, new KeyWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_Y|MINSG_COORDSYS_RELATIVE));
	eventHandler->setAction(Util::UI::KEY_LEFT, new KeyWrapper(camera, MINSG_ROTATE|MINSG_POSITIV_Y|MINSG_COORDSYS_RELATIVE));
	// mouse
	Wrapper *x = new MouseMotionWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_Y|MINSG_COORDSYS_RELATIVE);
	Wrapper *y = new MouseMotionWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_X|MINSG_COORDSYS_LOCAL);
	Wrapper *lbx = new MouseMotionWrapper(camera, MINSG_MOVE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL);
	Wrapper *lby = new MouseMotionWrapper(camera, MINSG_MOVE|MINSG_POSITIV_Z|MINSG_COORDSYS_LOCAL);
	Wrapper *rbx = new MouseMotionWrapper(camera, MINSG_MOVE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL);
	Wrapper *rby = new MouseMotionWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_Y|MINSG_COORDSYS_LOCAL);
	eventHandler->setAction(MINSG_MOUSE_BUTTON_MIDDLE, new ExchangeWrapper(eventHandler, x, y, lbx, lby, rbx, rby));
	// speedControl
	SpeedWrapper *splus = new SpeedWrapper(eventHandler, MINSG_MOVE, 1.5f);
	SpeedWrapper *sminus = new SpeedWrapper(eventHandler, MINSG_MOVE, 1/1.5f);
	eventHandler->setAction('+', splus);
	eventHandler->setAction('-', sminus);
	eventHandler->setAction(Util::UI::KEY_KPADD, splus);
	eventHandler->setAction(Util::UI::KEY_KPSUBTRACT, sminus);
	eventHandler->setAction(']', splus);
	eventHandler->setAction('/', sminus);
	// joystick / gamepad
	eventHandler->setAction(MINSG_JOY_BUTTON_6, splus);
	eventHandler->setAction(MINSG_JOY_BUTTON_8, sminus);
	eventHandler->setAction(MINSG_JOY_STICK_LEFT_X, new AnalogStickWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_Y|MINSG_COORDSYS_RELATIVE));
	eventHandler->setAction(MINSG_JOY_STICK_LEFT_Y, new AnalogStickWrapper(camera, MINSG_MOVE|MINSG_POSITIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_STICK_RIGHT_X, new AnalogStickWrapper(camera, MINSG_ROTATE|MINSG_NEGATIV_Y|MINSG_COORDSYS_RELATIVE));
	eventHandler->setAction(MINSG_JOY_STICK_RIGHT_Y, new AnalogStickWrapper(camera, MINSG_ROTATE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_HAT_DOWN, new ButtonWrapper(camera, MINSG_MOVE|MINSG_POSITIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_HAT_UP, new ButtonWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_Z|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_HAT_RIGHT, new ButtonWrapper(camera, MINSG_MOVE|MINSG_POSITIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_HAT_LEFT, new ButtonWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_X|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_BUTTON_5, new ButtonWrapper(camera, MINSG_MOVE|MINSG_POSITIV_Y|MINSG_COORDSYS_LOCAL));
	eventHandler->setAction(MINSG_JOY_BUTTON_7, new ButtonWrapper(camera, MINSG_MOVE|MINSG_NEGATIV_Y|MINSG_COORDSYS_LOCAL));
	// speed
	eventHandler->setSpeedFactor(MINSG_MOVE, 800);
}
