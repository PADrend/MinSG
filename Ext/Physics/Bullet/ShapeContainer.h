/*
	This file is part of the MinSG library extension Physics.
	Copyright (C) 2013 Mouns Almarrani
	Copyright (C) 2009-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2009-2013 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2009-2013 Ralf Petring <ralf@petring.net>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PHYSICS

#ifndef BTSHAPECONTAINER_H
#define BTSHAPECONTAINER_H

#include <Util/References.h>
#include <memory>

class btCollisionShape;

namespace MinSG {

namespace Physics {

class ShapeContainer : public Util::ReferenceCounter<ShapeContainer>{
        std::unique_ptr<btCollisionShape> shape;
    public:
        ShapeContainer(btCollisionShape* _shape ) :  Util::ReferenceCounter<ShapeContainer>(), shape(_shape){}
        ShapeContainer(const ShapeContainer&) = delete;
        ShapeContainer(ShapeContainer&&) = delete;
        ~ShapeContainer() = default;
        btCollisionShape* getShape()const { return shape.get();  }
};

}
}

#endif /* BTSHAPECONTAINER_H */

#endif /* MINSG_EXT_PHYSICS */
