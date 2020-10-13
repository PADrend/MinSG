/*
	This file is part of the MinSG library extension Behaviours.
	Copyright (C) 2011 Sascha Brandt
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2009-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2009-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_SOUND

#ifndef __SoundReceivingBehaviour_H
#define __SoundReceivingBehaviour_H

#include "../../Core/Behaviours/AbstractBehaviour.h"
#include <Sound/Listener.h>
#include <Geometry/Vec3.h>

namespace MinSG {

/**
 * SoundReceivingBehaviour ---|> AbstractNodeBehaviour
 * @ingroup behaviour
 */
class SoundReceivingBehaviour : public AbstractNodeBehaviour {
	PROVIDES_TYPE_NAME(SoundReceivingBehaviour)

	public:
		MINSGAPI SoundReceivingBehaviour(Node * node);
		virtual ~SoundReceivingBehaviour() {
		}

		MINSGAPI behaviourResult_t doExecute() override;

		Sound::Listener * getListener()const	{	return Sound::Listener::getInstance();	}

	private:
		Geometry::Vec3 lastPosition;
		timestamp_t lastTime;
};

}
#endif // __SoundReceivingBehaviour_H
#endif // MINSG_EXT_SOUND
