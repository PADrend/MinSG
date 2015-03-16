/*
	This file is part of the MinSG library.
	Copyright (C) 2010-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef EVENTHANDLER_H
#define EVENTHANDLER_H

#include <mutex>

namespace Util {
namespace UI {
union Event;
}
}

namespace MinSG {

    /***
     ** EventHandler
     **/
    class EventHandler {
        public:
            EventHandler();
            virtual ~EventHandler();

            bool process(const Util::UI::Event & e);
            void execute();

        protected:
            virtual bool processMouseEvent(const Util::UI::Event & e) =0;
            virtual bool processJoyEvent(const Util::UI::Event & e) =0;
            virtual bool processKeyEvent(const Util::UI::Event & e) =0;
            virtual void doExecute() =0;

        private:
			std::mutex mutex;
    };
}

#endif // EVENTHANDLER_H
