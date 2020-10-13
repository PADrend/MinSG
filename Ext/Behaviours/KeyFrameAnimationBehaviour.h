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
#ifndef __KeyFrameAnimationBehaviour_H
#define __KeyFrameAnimationBehaviour_H

#include "../../Core/Behaviours/AbstractBehaviour.h"

namespace MinSG {
class KeyFrameAnimationNode;

//! @ingroup behaviour
class KeyFrameAnimationBehaviour : public AbstractNodeBehaviour {
	PROVIDES_TYPE_NAME(KeyFrameAnimationBehaviour)

	public:
		MINSGAPI KeyFrameAnimationBehaviour(KeyFrameAnimationNode * node);
		MINSGAPI virtual ~KeyFrameAnimationBehaviour();

		// ---|> AbstractBehaviour
		MINSGAPI behaviourResult_t doExecute() override;

};

}
#endif //__KeyFrameAnimationBehaviour_H
