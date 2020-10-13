/*
	This file is part of the MinSG library extension OutOfCore.
	Copyright (C) 2011-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2011 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2011 Ralf Petring <ralf@petring.net>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_OUTOFCORE

#ifndef MESHATTRIBUTESERIALIZATION_H
#define MESHATTRIBUTESERIALIZATION_H

namespace MinSG{
namespace OutOfCore {

/*! Adds a handler for Util::_CounterAttribute<Mesh> to Util::GenericAttributeSerialization.
 S hould be called at least once before a *GenericAttribute is serialized which
 may contain a Mesh.
 \note Texture-Serialization may be added here when needed.
 \note The return value is always true and can be used for static initialization.
 */
MINSGAPI void initMeshAttributeSerialization();

}
}

#endif // MESHATTRIBUTESERIALIZATION_H
#endif // MINSG_EXT_OUTOFCORE
