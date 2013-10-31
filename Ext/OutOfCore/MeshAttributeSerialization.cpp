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

#include "MeshAttributeSerialization.h"
#include "OutOfCore.h"

#include <Rendering/Serialization/GenericAttributeSerialization.h>
#include <Rendering/Serialization/Serialization.h>
#include <Rendering/Mesh/Mesh.h>
#include <Util/GenericAttribute.h>
#include <Util/GenericAttributeSerialization.h>
#include <Util/Encoding.h>

namespace MinSG {
namespace OutOfCore {

static Rendering::Serialization::MeshAttribute_t * unserializeGAMesh(const std::pair<std::string, const Util::GenericAttributeMap *> & contentAndContext) {
	const std::string & s = contentAndContext.first;
	Rendering::Mesh * mesh = nullptr;
	if(s.compare(0, Rendering::Serialization::embeddedMeshPrefix.length(), Rendering::Serialization::embeddedMeshPrefix) == 0) {
		const std::vector<uint8_t> meshData = Util::decodeBase64(s.substr(Rendering::Serialization::embeddedMeshPrefix.length()));
		mesh = Rendering::Serialization::loadMesh("mmf", std::string(meshData.begin(), meshData.end()));
	} else {
		Geometry::Box invalidBox;
		invalidBox.invalidate();
		mesh = addMesh(Util::FileName(s), invalidBox);
	}
	return mesh == nullptr ? nullptr : new Rendering::Serialization::MeshAttribute_t(mesh);
}

void initMeshAttributeSerialization() {
	Util::GenericAttributeSerialization::registerSerializer<Rendering::Serialization::MeshAttribute_t>(
		Rendering::Serialization::GATypeNameMesh,
		Rendering::Serialization::serializeGAMesh,
		unserializeGAMesh
	);
}

}
}

#endif // MINSG_EXT_OUTOFCORE
