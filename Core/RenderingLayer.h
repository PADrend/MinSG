/*
	This file is part of the MinSG library.
	Copyright (C) 2014 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_RENDERING_LAYER_H
#define MINSG_RENDERING_LAYER_H

#include <cstdint>

namespace MinSG {

typedef uint8_t renderingLayerMask_t;
static const renderingLayerMask_t RENDERING_LAYER_DEFAULT = 1<<0;
static const renderingLayerMask_t RENDERING_LAYER_BUILTIN_META_OBJ = 1<<1;

}

#endif // MINSG_RENDERING_LAYER_H
