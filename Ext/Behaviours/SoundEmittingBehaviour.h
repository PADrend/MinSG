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

#ifndef __SoundEmittingBehaviour_H
#define __SoundEmittingBehaviour_H

#include "../../Core/Behaviours/AbstractBehaviour.h"
#include <Sound/Source.h>
#include <Geometry/Vec3.h>

namespace MinSG {

/**
 * SoundEmittingBehaviour ---|> AbstractNodeBehaviour
 * @ingroup behaviour
 */
class SoundEmittingBehaviour : public AbstractNodeBehaviour {
	PROVIDES_TYPE_NAME(SoundEmittingBehaviour)

	public:
		MINSGAPI SoundEmittingBehaviour(Node * node);
		MINSGAPI virtual ~SoundEmittingBehaviour();

		MINSGAPI behaviourResult_t doExecute() override;
		MINSGAPI void onInit() override;

		bool isRemoveWhenStopped()const		{	return removeWhenStopped;	}
		Sound::Source * getSource()const	{	return source.get();	}
		void setRemoveWhenStopped(bool b)	{	removeWhenStopped = b;	}

	private:
		MINSGAPI void doFinalize(BehaviorStatus &)override;

		Util::Reference<Sound::Source> source;
		bool removeWhenStopped;
		Geometry::Vec3 lastPosition;
		timestamp_t lastTime;
};

}
#endif // __SoundEmittingBehaviour_H
#endif // MINSG_EXT_SOUND
