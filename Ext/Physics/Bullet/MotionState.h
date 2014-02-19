/*
	This file is part of the MinSG library extension Physics.
	Copyright (C) 2013 Mouns Almarrani
	Copyright (C) 2013 Claudius Jähn <claudius@uni-paderborn.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PHYSICS

#ifndef PHYSICMOTIONSTATE_H
#define PHYSICMOTIONSTATE_H

#include"Helper.h"
#include"BtPhysicObject.h"


COMPILER_WARN_PUSH
COMPILER_WARN_OFF_CLANG(-W#warnings)
COMPILER_WARN_OFF_GCC(-Wswitch-default)
COMPILER_WARN_OFF_GCC(-Wunused-parameter)
COMPILER_WARN_OFF_GCC(-Wunused-variable)
COMPILER_WARN_OFF_GCC(-Wcast-qual)
COMPILER_WARN_OFF_GCC(-Woverloaded-virtual)
COMPILER_WARN_OFF_GCC(-Wshadow)
COMPILER_WARN_OFF_GCC(-Wold-style-cast)
#include <LinearMath/btDefaultMotionState.h>
COMPILER_WARN_POP

namespace MinSG {
    class Node;
namespace Physics {
class BtPhysicWorld;

class MotionState: public btMotionState{

		BtPhysicWorld& world;
		BtPhysicObject& physObj;
		const btTransform initialPos;

    public:
        //! create a new MotionState
        MotionState(BtPhysicWorld& _world, BtPhysicObject& _physObj, const btTransform& _initialpos):
			world(_world), physObj(_physObj), initialPos(_initialpos){}

        virtual ~MotionState(){}

        void getWorldTransform(btTransform &worldTrans) const override{
			worldTrans = initialPos;
        }
        void setWorldTransform(const btTransform &worldTrans) override;
};
}
}




#endif /* PHYSICMOTIONSTATE_H */

#endif /* MINSG_EXT_PHYSICS */
