/*
	This file is part of the MinSG library.
	Copyright (C) 2010-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2010 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ActionWrapper.h"
#include "MoveNodeHandler.h"

using namespace std;
using namespace MinSG;

namespace MinSG {

/// SpeedWrapper

	SpeedWrapper::SpeedWrapper(MoveNodeHandler *_handler, int _action, float _factor) : Wrapper(), handler(_handler), action(_action), factor(_factor) {}

	SpeedWrapper::~SpeedWrapper() {}

	void SpeedWrapper::setSpeedFactor(int /*action*/, float /*f*/) {}

	void SpeedWrapper::incSpeedFactor(int /*action*/, float /*f*/) {}

	void SpeedWrapper::setSpeed(float i) {
		if (i!=0) {
			handler->incSpeedFactor(action, factor);
		}
	}

	void SpeedWrapper::execute() {
	}


/// ExchangeWrapper

	ExchangeWrapper::ExchangeWrapper(MoveNodeHandler *_handler, Wrapper *_x, Wrapper *_y, Wrapper *_lbx, Wrapper *_lby, Wrapper *_rbx, Wrapper *_rby):
			Wrapper(), enabled(false), handler(_handler), x(_x), y(_y), lbx(_lbx), lby(_lby), rbx(_rbx), rby(_rby) {
	}

	ExchangeWrapper::~ExchangeWrapper() {
	}

	void ExchangeWrapper::setSpeed(float /*i*/) {
// 		buttonState = SDL_GetMouseState(NULL, NULL);
// 		if (buttonState & SDL_BUTTON_MMASK) {
// 			enabled = !enabled;
// 			buttonState ^= SDL_BUTTON_MMASK;
// 			if (enabled) {
// 				SDL_ShowCursor(SDL_DISABLE);
// 				SDL_WM_GrabInput(SDL_GRAB_ON);
// 				handler->setAction(MINSG_MOUSE_BUTTON_LEFT, this);
// 				handler->setAction(MINSG_MOUSE_BUTTON_RIGHT, this);
// 			} else {
// 				SDL_ShowCursor(SDL_ENABLE);
// 				SDL_WM_GrabInput(SDL_GRAB_OFF);
// 				handler->setAction(MINSG_MOUSE_BUTTON_LEFT, 0);
// 				handler->setAction(MINSG_MOUSE_BUTTON_RIGHT, 0);
// 				handler->setAction(MINSG_MOUSE_MOTION_X, 0);
// 				handler->setAction(MINSG_MOUSE_MOTION_Y, 0);
// 			}
// 		}
// 		if (enabled) {
// 			switch (buttonState) {
// 				case SDL_BUTTON_LMASK:
// 					handler->setAction(MINSG_MOUSE_MOTION_X, lbx);
// 					handler->setAction(MINSG_MOUSE_MOTION_Y, lby);
// 					break;
// 				case SDL_BUTTON_RMASK:
// 					handler->setAction(MINSG_MOUSE_MOTION_X, rbx);
// 					handler->setAction(MINSG_MOUSE_MOTION_Y, rby);
// 					break;
// 				case 0: // no Button
// 					handler->setAction(MINSG_MOUSE_MOTION_X, x);
// 					handler->setAction(MINSG_MOUSE_MOTION_Y, y);
// 					break;
// 				default:
// 					handler->setAction(MINSG_MOUSE_MOTION_X, 0);
// 					handler->setAction(MINSG_MOUSE_MOTION_Y, 0);
// 					break;
// 			}
// 		}
	}

	void ExchangeWrapper::setSpeedFactor(int _action, float f) {
		if (x) x->setSpeedFactor(_action, f);
		if (y) y->setSpeedFactor(_action, f);
		if (lbx) lbx->setSpeedFactor(_action, f);
		if (lby) lby->setSpeedFactor(_action, f);
		if (rbx) rbx->setSpeedFactor(_action, f);
		if (rby) rby->setSpeedFactor(_action, f);
	}

	void ExchangeWrapper::incSpeedFactor(int _action, float f) {
		if (x) x->incSpeedFactor(_action, f);
		if (y) y->incSpeedFactor(_action, f);
		if (lbx) lbx->incSpeedFactor(_action, f);
		if (lby) lby->incSpeedFactor(_action, f);
		if (rbx) rbx->incSpeedFactor(_action, f);
		if (rby) rby->incSpeedFactor(_action, f);
	}

	void ExchangeWrapper::execute() {
	}


/// MotionWrapper

	void MotionWrapper::updateValue() {
		if (timerUsed){
			value += timer.getSeconds()*speed*speedFactor;
			timer.reset();
		}else{
			value += speed*speedFactor;
		}
	}

	MotionWrapper::MotionWrapper(Node *_node, int _action, bool _timerUsed) : Wrapper(), value(0), speedFactor(1), timerUsed(_timerUsed), node(_node), action(_action), speed(0) {
		if (timerUsed)
			timer.reset();

		if (action & MINSG_NEGATIV_X) vector = Geometry::Vec3(-1,0,0);
		if (action & MINSG_POSITIV_X) vector = Geometry::Vec3(1,0,0);
		if (action & MINSG_NEGATIV_Y) vector = Geometry::Vec3(0,-1,0);
		if (action & MINSG_POSITIV_Y) vector = Geometry::Vec3(0,1,0);
		if (action & MINSG_NEGATIV_Z) vector = Geometry::Vec3(0,0,-1);
		if (action & MINSG_POSITIV_Z) vector = Geometry::Vec3(0,0,1);
	}

	MotionWrapper::~MotionWrapper() {
	}

	float MotionWrapper::getSpeed() {
		return speed;
	}

	void MotionWrapper::setSpeedFactor(int _action, float f) {
		if (_action & action)
			speedFactor = f;
	}

	void MotionWrapper::incSpeedFactor(int _action, float f) {
		if (_action & action)
			speedFactor *= f;
	}

	float MotionWrapper::getValue() {
		updateValue();
		float ret = value;
		value = 0;
		return ret;
	}

	void MotionWrapper::execute() {
		float v = getValue();
		if (v!=0) {
			if (action & MINSG_ROTATE) {
				if (action & MINSG_COORDSYS_LOCAL) {
					node->rotateLocal_rad(v, vector);
				} else if (action & MINSG_COORDSYS_RELATIVE) {
					node->rotateRel_rad(v, vector);
				}
			} else if (action & MINSG_MOVE) {
				if (action & MINSG_COORDSYS_LOCAL) {
					node->moveLocal(vector*v);
				} else if (action & MINSG_COORDSYS_RELATIVE) {
					node->moveRel(vector*v);
				}
			}
		}
	}


/// AnalogStickWrapper

	AnalogStickWrapper::AnalogStickWrapper(Node * _node, int _action, float _maxSpeed) : MotionWrapper(_node, _action, true), maxSpeed(_maxSpeed) {
	}

	AnalogStickWrapper::~AnalogStickWrapper() {
	}

	void AnalogStickWrapper::setMaxSpeed(int i) {
		maxSpeed = (float)i;
	}

	void AnalogStickWrapper::setSpeed(float f) {
		updateValue();
		if (f < maxSpeed/30 && f > -maxSpeed/30)
			f=0;
		speed = max(min(f/maxSpeed,1.0f),-1.0f);
		if(f<0)
            speed *= -speed;
        else
            speed *= speed;
	}


/// ButtonWrapper (typedef KeyWrapper)

	ButtonWrapper::ButtonWrapper(Node * _node, int _action) : MotionWrapper(_node, _action, true) {
	}

	ButtonWrapper::~ButtonWrapper() {
	}

	void ButtonWrapper::setSpeed(float f) {
		updateValue();
		speed = f;
	}


/// MouseMotionWrapper

	MouseMotionWrapper::MouseMotionWrapper(Node * _node, int _action) : MotionWrapper(_node, _action, false) {
	}

	MouseMotionWrapper::~MouseMotionWrapper() {
	}

	void MouseMotionWrapper::setSpeed(float f) {
		speed = f;
		if (action & MINSG_ROTATE)
			speed/=200;
        if (action & MINSG_MOVE)
            speed /=200;
		updateValue();
		speed = 0;
	}
}
