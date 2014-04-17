/*
	This file is part of the MinSG library extension OutOfCore.
	Copyright (C) 2011-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2011 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_OUTOFCORE

#include "ImportHandler.h"
#include "OutOfCore.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../SceneManagement/SceneDescription.h"
#include <Geometry/Box.h>
#include <Geometry/Vec3.h>
#include <Rendering/Mesh/Mesh.h>
#include <Util/IO/FileName.h>
#include <Util/IO/FileLocator.h>
#include <Util/StringUtils.h>
#include <Util/Macros.h>
#include <vector>
#include <string>

namespace Rendering {
class Mesh;
}
namespace MinSG {
class Node;
namespace OutOfCore {

Node * ImportHandler::handleImport(const Util::FileLocator& locator,const std::string & filename, const SceneManagement::NodeDescription * description) {
	const std::string meshBBString = description->getString(SceneManagement::Consts::ATTR_MESH_BB);
	if (meshBBString.empty()) {
		WARN("Found mesh that is not handled here (bounding box is not available).");
		return SceneManagement::MeshImportHandler::handleImport(locator, filename, description);
	}
	const auto location = locator.locateFile(Util::FileName(filename));
	const Util::FileName url = location.second; // continue even if the existence of the file could not be verified (location.first==false)
	
	if ((url.getEnding() != "mmf" && url.getEnding() != "ply")) {
		WARN("Found mesh that is not handled here (\"" + url.toString() + "\" does not end with \"mmf\" or \"ply\").");
		return SceneManagement::MeshImportHandler::handleImport(locator, filename, description);
	}

	const auto boxValues = Util::StringUtils::toFloats(meshBBString);
	FAIL_IF(boxValues.size() != 6);
	const Geometry::Box meshBB(Geometry::Vec3(boxValues[0], boxValues[1], boxValues[2]), boxValues[3], boxValues[4], boxValues[5]);

	Rendering::Mesh * mesh = addMesh(url, meshBB); // this can mess up the filename... solution is too complex.
	return new GeometryNode(mesh);
}

}
}

#endif /* MINSG_EXT_OUTOFCORE */
