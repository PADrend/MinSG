/*
	This file is part of the MinSG library.
	Copyright (C) 2010-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MOVENODEHANDLER_H
#define MOVENODEHANDLER_H

#include "EventHandler.h"
#include "ActionWrapper.h"

#include <map>

namespace MinSG {

    using namespace MinSG;
    using namespace std;

    /***
     ** MoveNodeHandler ---|> EventHandler
     **/
	class MoveNodeHandler : public EventHandler {

		private:
			/// mysterious bug: all consts have to be int, map has to be unsigned
			map<unsigned int, Wrapper*> wrapper;
			int rotateMode;

			int joystickId;

        protected:

			virtual bool processMouseEvent(const Util::UI::Event & e) ;
			virtual bool processJoyEvent(const Util::UI::Event & e);
			virtual bool processKeyEvent(const Util::UI::Event & e);
			virtual void doExecute();

		public:

			MoveNodeHandler();
			virtual ~MoveNodeHandler();

			void setAction(const int trigger, Wrapper *w) ;
			void setJoystick(const int id);
			void setSpeedFactor(const int action , const float s);
			void incSpeedFactor(const int action, const float factor = 1.4);

			static void initRalf(MoveNodeHandler *eventHandler, Node *camera);
			static void initClaudius(MoveNodeHandler *eventHandler, Node *camera);
			static void initStephan(MoveNodeHandler *eventHandler, Node *camera);
			static void initAdrian(MoveNodeHandler *eventHandler, Node *camera);
	};
}

#endif // MOVENODEHANDLER_H
