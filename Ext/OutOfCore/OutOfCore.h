/*
	This file is part of the MinSG library extension OutOfCore.
	Copyright (C) 2011-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_OUTOFCORE

#ifndef OUTOFCORE_H_
#define OUTOFCORE_H_

namespace Geometry {
template<typename value_t> class _Box;
typedef _Box<float> Box;
}
namespace Rendering {
class Mesh;
}
namespace Util {
class FileName;
}
namespace MinSG {
class FrameContext;

//! @ingroup ext
namespace OutOfCore {
class CacheManager;
class DataStrategy;

//! Return the single instance of CacheManager.
MINSGAPI CacheManager & getCacheManager();

//! Return the single instance of DataStrategy.
MINSGAPI DataStrategy & getDataStrategy();

//! Associate the out-of-core system to the FrameContext (so that it is triggered every frame) and register the MeshImport function.
MINSGAPI void setUp(FrameContext& context);

//! Remove the association of the out-of-core system and remove all cache levels.
MINSGAPI void shutDown();

//! Return @c true, if setUp() has been called at least once.
MINSGAPI bool isSystemEnabled();

//! Helper function to add a new mesh to the out-of-core system.
MINSGAPI Rendering::Mesh * addMesh(const Util::FileName & meshFile, const Geometry::Box & meshBB);

}
}

#endif /* OUTOFCORE_H_ */

#endif /* MINSG_EXT_OUTOFCORE */
