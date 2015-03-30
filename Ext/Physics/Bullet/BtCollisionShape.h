/*
	This file is part of the MinSG library extension Physics.
	Copyright (C) 2013 Mouns Almarrani
	Copyright (C) 2009-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2009-2015 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2009-2013 Ralf Petring <ralf@petring.net>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PHYSICS

#ifndef BTSHAPECONTAINER_H
#define BTSHAPECONTAINER_H

#include "../CollisionShape.h"
#include <memory>

class btCollisionShape;

namespace MinSG {

namespace Physics {

class BtCollisionShape : public CollisionShape{
        std::unique_ptr<btCollisionShape> shape;
    public:
        BtCollisionShape(btCollisionShape* _shape) : CollisionShape(), shape(_shape){}
        BtCollisionShape(const BtCollisionShape&) = delete;
        BtCollisionShape(BtCollisionShape&&) = delete;
        virtual ~BtCollisionShape(){};
        btCollisionShape* getShape()const { return shape.get();  }
};

}
}

#endif /* BTSHAPECONTAINER_H */

#endif /* MINSG_EXT_PHYSICS */
