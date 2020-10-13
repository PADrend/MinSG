/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef EnvironmentState_H
#define EnvironmentState_H

#include "../../Core/States/State.h"

namespace MinSG {

/**
 *  A State for displaying an environment at great distance around the observer's
 *  position (like a skydome).
 *  When activated, the environment is displayed at the current position with
 *  a deactivated depth mask and deactivated depth testing.
 *  [EnvironmentState] ---|> [State]
 * @ingroup states
 */
class EnvironmentState : public State {
		PROVIDES_TYPE_NAME(EnvironmentState)
	public:

		MINSGAPI EnvironmentState(Node * environment=nullptr);
		MINSGAPI virtual ~EnvironmentState();

		void setEnvironment(Node * n)   {   environment=n;  }
		Node * getEnvironment()const    {   return environment.get();   }

		// ---|> [State]
		MINSGAPI EnvironmentState * clone() const override;

	private:
		Util::Reference<Node> environment;

		MINSGAPI stateResult_t doEnableState(FrameContext & context,Node *, const RenderParam & rp) override;
};

}

#endif // EnvironmentState_H
