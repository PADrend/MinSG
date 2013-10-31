/*
	This file is part of the MinSG library.
	Copyright (C) 2010-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef ACTIONWRAPPER_H
#define ACTIONWRAPPER_H

#include <MinSG/Core/Nodes/Node.h>
#include <Util/Timer.h>

namespace MinSG {

    using namespace std;
	static const int ACTION_OFFSET = 10000;

	static const int MINSG_JOY_BUTTON_1 = ACTION_OFFSET + 1;
	static const int MINSG_JOY_BUTTON_2 = ACTION_OFFSET + 2;
	static const int MINSG_JOY_BUTTON_3 = ACTION_OFFSET + 3;
	static const int MINSG_JOY_BUTTON_4 = ACTION_OFFSET + 4;
	static const int MINSG_JOY_BUTTON_5 = ACTION_OFFSET + 5;
	static const int MINSG_JOY_BUTTON_6 = ACTION_OFFSET + 6;
	static const int MINSG_JOY_BUTTON_7 = ACTION_OFFSET + 7;
	static const int MINSG_JOY_BUTTON_8 = ACTION_OFFSET + 8;
	static const int MINSG_JOY_BUTTON_9 = ACTION_OFFSET + 9;
	static const int MINSG_JOY_BUTTON_10 = ACTION_OFFSET + 10;
	static const int MINSG_JOY_BUTTON_11 = ACTION_OFFSET + 11;
	static const int MINSG_JOY_BUTTON_12 = ACTION_OFFSET + 12;

	static const int MINSG_MOUSE_BUTTON_LEFT = ACTION_OFFSET + 13;
	static const int MINSG_MOUSE_BUTTON_MIDDLE = ACTION_OFFSET + 14;
	static const int MINSG_MOUSE_BUTTON_RIGHT = ACTION_OFFSET + 15;
	static const int MINSG_MOUSE_BUTTON_WHEELDOWN = ACTION_OFFSET + 16;
	static const int MINSG_MOUSE_BUTTON_WHEELUP = ACTION_OFFSET + 17;

	static const int MINSG_JOY_HAT_UP = ACTION_OFFSET + 18;
	static const int MINSG_JOY_HAT_RIGHT = ACTION_OFFSET + 19;
	static const int MINSG_JOY_HAT_DOWN = ACTION_OFFSET + 20;
	static const int MINSG_JOY_HAT_LEFT = ACTION_OFFSET + 21;

	static const int MINSG_JOY_STICK_LEFT_X = ACTION_OFFSET + 22;
	static const int MINSG_JOY_STICK_LEFT_Y = ACTION_OFFSET + 23;
	static const int MINSG_JOY_STICK_RIGHT_X = ACTION_OFFSET + 24;
	static const int MINSG_JOY_STICK_RIGHT_Y = ACTION_OFFSET + 25;

	static const int MINSG_MOUSE_MOTION_X = ACTION_OFFSET + 26;
	static const int MINSG_MOUSE_MOTION_Y = ACTION_OFFSET + 27;

	// Actions
	static const int MINSG_POSITIV_X = 1<<0;
	static const int MINSG_NEGATIV_X = 1<<1;
	static const int MINSG_POSITIV_Y = 1<<2;
	static const int MINSG_NEGATIV_Y = 1<<3;
	static const int MINSG_POSITIV_Z = 1<<4;
	static const int MINSG_NEGATIV_Z = 1<<5;
	static const int MINSG_MOVE = 1<<6;
	static const int MINSG_ROTATE = 1<<7;
	static const int MINSG_COORDSYS_RELATIVE = 1<<9;
	static const int MINSG_COORDSYS_LOCAL = 1<<10;

	class MoveNodeHandler;

    /***
     ** Wrapper
     **/
	class Wrapper {
		public:
			Wrapper(){};
			virtual ~Wrapper(){};

			virtual void setSpeedFactor(int action, float f)=0;
			virtual void incSpeedFactor(int action, float f)=0;
			virtual void setSpeed(float i) = 0;
			virtual void execute() = 0;
	};

    /***
     ** SpeedWrapper ---|> Wrapper
     **/
	class SpeedWrapper :public Wrapper{
        private:
            MoveNodeHandler *handler;
            int action;
            float factor;
	    public:
            SpeedWrapper(MoveNodeHandler *handler, int action = MINSG_MOVE, float factor = 1.5f);
            virtual ~SpeedWrapper();

			virtual void setSpeedFactor(int action, float f);
			virtual void incSpeedFactor(int action, float f);
			virtual void setSpeed(float i);
			virtual void execute();
	};

    /***
     ** ExchangeWrapper ---|> Wrapper
     **/
	class ExchangeWrapper : public Wrapper {
		private:
            bool enabled;
			int buttonState;
			MoveNodeHandler *handler;
			Wrapper *x, *y, *lbx, *lby, *rbx, *rby;
		public:
			ExchangeWrapper(MoveNodeHandler *_handler, Wrapper *_x, Wrapper *_y, Wrapper *_lbx, Wrapper *_lby, Wrapper *_rbx, Wrapper *_rby);
			virtual ~ExchangeWrapper();
			virtual void setSpeed(float i);
			virtual void execute();
			virtual void setSpeedFactor(int action, float f);
			virtual void incSpeedFactor(int action, float f);
	};

    /***
     ** MotionWrapper ---|> Wrapper
     **/
	class MotionWrapper : public Wrapper {
		private:
			Geometry::Vec3 vector;
			Util::Timer timer;
			float value;
			float speedFactor;
			bool timerUsed;
		protected:
			Node *node;
			int action;
			float speed;
		public:
			MotionWrapper(Node *node, int _action, bool _timerUsed);
			virtual ~MotionWrapper();
			virtual void execute();
			virtual void setSpeedFactor(int action, float f);
			virtual void incSpeedFactor(int action, float f);
		protected:
			virtual void updateValue();
			float getSpeed();
			float getValue();
	};

    /***
     ** AnalogStickWrapper ---|> MotionWrapper ---|> Wrapper
     **/
	class AnalogStickWrapper : public MotionWrapper {
		public:
			float maxSpeed;

			AnalogStickWrapper(Node *node, int action, float _maxSpeed = 32768.0f);
			virtual ~AnalogStickWrapper();
			void setMaxSpeed(int i);
			virtual void setSpeed(float f);
	};

    /***
     ** ButtonWrapper ---|> MotionWrapper ---|> Wrapper
     **/
     class ButtonWrapper : public MotionWrapper {
		public :
			ButtonWrapper(Node * _node, int _action);
			virtual ~ButtonWrapper();
			virtual void setSpeed(float f);
	};

	typedef ButtonWrapper KeyWrapper;

    /***
     ** MouseMotionWrapper ---|> MotionWrapper ---|> Wrapper
     **/
	class MouseMotionWrapper : public MotionWrapper {
		public :
			MouseMotionWrapper(Node * _node, int _action);
			virtual ~MouseMotionWrapper();
			virtual void setSpeed(float f);
	};
}

#endif //ACTIONWRAPPER_H
