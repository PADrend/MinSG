/*
	This file is part of the MinSG library extension Physics.
	Copyright (C) 2015 Claudius Jähn <claudius@uni-paderborn.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PHYSICS

#ifndef PHYSICCOLLSIONSHAPE_H
#define PHYSICCOLLSIONSHAPE_H

#include <Util/ReferenceCounter.h>

namespace MinSG {
namespace Physics {

class CollisionShape : public Util::ReferenceCounter<CollisionShape>{
	public:
		CollisionShape() = default;
		CollisionShape(CollisionShape&&)=default;
		virtual ~CollisionShape(){}
};

}
}

#endif /* PHYSICCOLLSIONSHAPE_H */

#endif /* MINSG_EXT_PHYSICS */
