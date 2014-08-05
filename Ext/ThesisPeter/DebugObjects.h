/*
	This file is part of the MinSG library extension ThesisPeter.
	Copyright (C) 2014 Peter Stilow

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#define MINSG_EXT_THESISPETER
#ifdef MINSG_EXT_THESISPETER

#ifndef MINSG_THESISPETER_DEBUGOBJECTS_H_
#define MINSG_THESISPETER_DEBUGOBJECTS_H_

#include <vector>
#include <Geometry/Vec3.h>
#include <Util/Graphics/ColorLibrary.h>
#include <MinSG/Core/Nodes/GeometryNode.h>
#include <MinSG/Core/Nodes/GroupNode.h>

namespace MinSG {
namespace ThesisPeter {

struct DebugLine {
	Geometry::Vec3 start;
	Geometry::Vec3 end;
	Util::Color4f colorStart;
	Util::Color4f colorEnd;
};

class DebugObjects {
public:
	DebugObjects();
	~DebugObjects();

	void setSceneRootNode(MinSG::Node* sceneRootNode);
	void addDebugLine(Geometry::Vec3 start, Geometry::Vec3 end, Util::Color4f colStart = Util::Color4f(1.0, 1.0, 1.0, 1.0), Util::Color4f colEnd = Util::Color4f(0.0, 0.0, 1.0, 1.0));
	void clearDebug();
	void buildDebugLineNode();
private:
	Util::Reference<MinSG::Node> lineNode;
	std::vector<DebugLine*> linesData;
	bool nodeAdded;
	MinSG::GroupNode *sceneRootNode;
};

}
}

#endif // MINSG_THESISPETER_DEBUGOBJECTS_H_

#endif // MINSG_EXT_THESISPETER
