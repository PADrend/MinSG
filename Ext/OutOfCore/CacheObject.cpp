/*
	This file is part of the MinSG library extension OutOfCore.
	Copyright (C) 2011-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_OUTOFCORE

#include "CacheObject.h"
#include <Rendering/Mesh/Mesh.h>

namespace MinSG {
namespace OutOfCore {

CacheObject::CacheObject(Rendering::Mesh * mesh) :
	content(mesh), priority(), highestLevelStored(0), updated(true) {
}

CacheObject::~CacheObject() = default;

}
}

#endif /* MINSG_EXT_OUTOFCORE */
