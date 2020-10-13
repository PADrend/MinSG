/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_SCENEMANAGEMENT_READERDAE_H
#define MINSG_SCENEMANAGEMENT_READERDAE_H

#include <iosfwd>

namespace Util {
class GenericAttributeMap;
}
namespace MinSG {
namespace SceneManagement {
typedef Util::GenericAttributeMap DescriptionMap;
namespace ReaderDAE {

/**
 * Load a COLLAborative Design Activity (COLLADA) stored inside a
 * Digital Asset Exchange (DAE) XML document.
 *
 * @param in Input stream containing the scene data
 * @param invertTransparency If @c true, transparency values are interpreted as
 * opacity values
 * @return Description of the loaded scene
 * @author Benjamin Eikel
 * @date 2009-08-03
 * @see http://www.khronos.org/collada/
 */
MINSGAPI const DescriptionMap * loadScene(std::istream & in, bool invertTransparency);

}
}
}

#endif /* MINSG_SCENEMANAGEMENT_READERDAE_H */
