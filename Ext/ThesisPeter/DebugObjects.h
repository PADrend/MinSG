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

struct DebugFace {
	Geometry::Vec3 vert1;
	Geometry::Vec3 vert2;
	Geometry::Vec3 vert3;
	Geometry::Vec3 vert4;
	Util::Color4f col1;
	Util::Color4f col2;
	Util::Color4f col3;
	Util::Color4f col4;
};

class DebugObjects {
public:
	DebugObjects();
	~DebugObjects();

	void setSceneRootNode(MinSG::Node* sceneRootNode);
	void addDebugLine(Geometry::Vec3 start, Geometry::Vec3 end, Util::Color4f colStart = Util::Color4f(1.0, 1.0, 1.0, 1.0), Util::Color4f colEnd = Util::Color4f(0.0, 0.0, 1.0, 1.0));
	void addDebugFace(Geometry::Vec3 vert1, Geometry::Vec3 vert2, Geometry::Vec3 vert3, Geometry::Vec3 vert4, Util::Color4f col1 = Util::Color4f(1.0, 1.0, 1.0, 1.0), Util::Color4f col2 = Util::Color4f(1.0, 1.0, 1.0, 1.0), Util::Color4f col3 = Util::Color4f(1.0, 1.0, 1.0, 1.0), Util::Color4f col4 = Util::Color4f(1.0, 1.0, 1.0, 1.0));
	void addDebugBoxMinMax(Geometry::Vec3 min, Geometry::Vec3 max, Util::Color4f col1 = Util::Color4f(1.0, 1.0, 1.0, 0.4), Util::Color4f col2 = Util::Color4f(1.0, 1.0, 1.0, 0.4));
	void addDebugBox(Geometry::Vec3 midPos, Geometry::Vec3 size, Util::Color4f col1 = Util::Color4f(1.0, 1.0, 1.0, 0.4), Util::Color4f col2 = Util::Color4f(1.0, 1.0, 1.0, 0.4));
	void addDebugBox(Geometry::Vec3 midPos, float size, Util::Color4f col1 = Util::Color4f(1.0, 1.0, 1.0, 0.4), Util::Color4f col2 = Util::Color4f(1.0, 1.0, 1.0, 0.4));
	void addDebugBoxLinesMinMax(Geometry::Vec3 min, Geometry::Vec3 max, Util::Color4f col1 = Util::Color4f(1.0, 1.0, 1.0, 0.4), Util::Color4f col2 = Util::Color4f(1.0, 1.0, 1.0, 0.4));
	void addDebugBoxLines(Geometry::Vec3 midPos, Geometry::Vec3 size, Util::Color4f col1 = Util::Color4f(1.0, 1.0, 1.0, 0.4), Util::Color4f col2 = Util::Color4f(1.0, 1.0, 1.0, 0.4));
	void addDebugBoxLines(Geometry::Vec3 midPos, float size, Util::Color4f col1 = Util::Color4f(1.0, 1.0, 1.0, 0.4), Util::Color4f col2 = Util::Color4f(1.0, 1.0, 1.0, 0.4));
	void clearDebug();
	void buildDebugLineNode();
	void buildDebugFaceNode();
private:
	Util::Reference<MinSG::Node> lineNode;
	Util::Reference<MinSG::Node> faceNode;
	std::vector<DebugLine*> linesData;
	std::vector<DebugFace*> facesData;
	bool nodeAddedLines, nodeAddedFaces;
	MinSG::GroupNode *sceneRootNode;
};

}
}

#endif // MINSG_THESISPETER_DEBUGOBJECTS_H_

#endif // MINSG_EXT_THESISPETER
