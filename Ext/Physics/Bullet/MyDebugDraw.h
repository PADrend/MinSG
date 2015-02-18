/*
	This file is part of the MinSG library extension Physics.
	Copyright (C) 2013 Mouns Almarrani
	Copyright (C) 2013 Claudius Jähn <claudius@uni-paderborn.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PHYSICS

#include "Helper.h"

#include <Rendering/RenderingContext/RenderingContext.h>
#include <Util/Graphics/ColorLibrary.h>
#include <Rendering/Draw.h>

#include<Util/Macros.h>
COMPILER_WARN_PUSH
COMPILER_WARN_OFF_CLANG(-W#warnings)
COMPILER_WARN_OFF_GCC(-Wswitch-default)
COMPILER_WARN_OFF_GCC(-Wunused-variable)
COMPILER_WARN_OFF_GCC(-Wunused-parameter)
COMPILER_WARN_OFF_GCC(-Woverloaded-virtual)
COMPILER_WARN_OFF_GCC(-Wshadow)
COMPILER_WARN_OFF_GCC(-Wold-style-cast)
#include <btBulletDynamicsCommon.h>
#if (BT_BULLET_VERSION == 282) and !defined(BULLET_WARNING_PATCH)
#define BULLET_WARNING_PATCH
inline int _suppressUnusedVariableWarning(){  return btInfinityMask;} // on mingw, -Wunused-variable does not work here.
#endif
COMPILER_WARN_POP

namespace MinSG {
    class Node;
namespace Physics {
class MyDebugDraw : public btIDebugDraw{
    private:
        int debugMode;
        Rendering::RenderingContext& renderingContext;

    public:
		MyDebugDraw(Rendering::RenderingContext& ctxt) : renderingContext(ctxt){}

		void setDebugMode(int _debugMode) override{
			debugMode = _debugMode;
		}

		int getDebugMode() const override{
			return debugMode;
		}

		void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color)override{
			Rendering::drawVector(renderingContext, toVec3(from), toVec3(to), toColor4f(color));
		}
		void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color1,const btVector3 &color2 )override{
			Rendering::drawVector(renderingContext, toVec3(from), toVec3(to), toColor4f(color1), toColor4f(color2));
		}
//		void drawContactPoint (const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color)override{}
		void drawContactPoint (const btVector3 &, const btVector3 &, btScalar , int , const btVector3 &)override{}
//		void reportErrorWarning (const char *warningString)override{}
		void reportErrorWarning (const char *)override{}
//		void draw3dText (const btVector3 &location, const char *textString)override{}
		void draw3dText (const btVector3 &, const char *)override{}

};

}
}
#endif
