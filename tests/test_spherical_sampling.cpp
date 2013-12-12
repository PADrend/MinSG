/*
	This file is part of the MinSG library.
	Copyright (C) 2012-2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include <Geometry/Box.h>
#include <Geometry/Vec3.h>
#include <MinSG/Core/Nodes/GeometryNode.h>
#include <MinSG/Core/Nodes/ListNode.h>
#include <MinSG/Core/FrameContext.h>
#include <MinSG/Ext/SVS/Helper.h>
#include <MinSG/Ext/SVS/PreprocessingContext.h>
#include <MinSG/Ext/SVS/SamplingSphere.h>
#include <MinSG/Ext/VisibilitySubdivision/VisibilityVector.h>
#include <MinSG/Helper/Helper.h>
#include <MinSG/Helper/StdNodeVisitors.h>
#include <MinSG/SceneManagement/SceneManager.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/MeshUtils/MeshBuilder.h>
#include <Util/References.h>
#include <Util/Timer.h>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <vector>

// Needed only for debug output
//#include <MinSG/Helper/GraphVizOutput.h>

// Prevent warning
int test_spherical_sampling();

int test_spherical_sampling() {
#ifdef MINSG_EXT_SVS
	std::cout << "Test SVS ... ";
	Util::Timer timer;
	timer.reset();

	MinSG::SceneManagement::SceneManager sceneManager;
	MinSG::FrameContext frameContext;

	Rendering::VertexDescription vertexDesc;
	vertexDesc.appendPosition3D();
	Util::Reference<Rendering::Mesh> boxMesh = Rendering::MeshUtils::MeshBuilder::createBox(vertexDesc, Geometry::Box(Geometry::Vec3f(0.5f, 0.5f, 0.5f), 1));

	/*
	 * Create a 1D array of n unit-size boxes in the x axis
	 *
	 *          /
	 *        ..L2
	 *        /  /
	 *    ......L1  /
	 *    /    /  /
	 *  ...L0....  /  /
	 *  /  /  /  /  /
	 * ------------------------- ... -----
	 * | 0 | 1 | 2 | 3 | 4 | 5 |   | n |
	 * |  |  |  |  |  |  |   |  |
	 * ------------------------- ... -----
	 * ^        ^       ^
	 * |        |       |
	 * x=0       x=4      x=n
	 *
	 * The first three boxes are put into the first list node L0.
	 * L0 together with the fourth box are put into the second list node L1.
	 */
	const uint32_t count = 512;

	std::vector<Util::Reference<MinSG::ListNode>> listNodes;
	std::vector<Util::Reference<MinSG::GeometryNode>> boxNodes;
	for(uint_fast32_t x = 0; x < count; ++x) {
		MinSG::GeometryNode * geoNode = new MinSG::GeometryNode(boxMesh);
		geoNode->moveLocal(Geometry::Vec3f(x, 0, 0));
		sceneManager.registerNode(std::string("Node") + Util::StringUtils::toString(x), geoNode);
		boxNodes.push_back(geoNode);
		if(x != 1 && x != 2) {
			listNodes.push_back(new MinSG::ListNode);
		}
		if(x < 3) {
			listNodes.front()->addChild(geoNode);
		} else {
			listNodes[x - 2]->addChild(listNodes[x - 3].get());
			listNodes[x - 2]->addChild(geoNode);
		}
	}

	// Debug output of created scene graph
	//MinSG::GraphVizOutput::treeToFile(listNodes.back().get(), &sceneManager, Util::FileName("output.dot"));

	// Perform the sampling
	std::vector<Geometry::Vec3f> positions;
	positions.emplace_back(-1, 0, 0); // left
	positions.emplace_back(1, 0, 0); // right
	positions.emplace_back(0, 1, 0); // top
	positions.emplace_back(0, 0, 1); // back

	// The resolution has to be at least the number of boxes. Otherwise, boxes will be missed during visibility testing.
	MinSG::SVS::PreprocessingContext preprocessingContext(sceneManager, frameContext, listNodes.back().get(), positions, count, false, false);
	while(!preprocessingContext.isFinished()) {
		preprocessingContext.preprocessSingleNode();
	}

	// Validate the results
	for(uint_fast32_t listIndex = 0; listIndex < count - 2; ++listIndex) {
		MinSG::ListNode * listNode = listNodes[listIndex].get();
		const MinSG::SVS::SamplingSphere & samplingSphere = MinSG::SVS::retrieveSamplingSphere(listNode);
		{
			// Because the geometry is built of axis-aligned bounding boxes only, the sphere has to contain the group's bounding box.
			assert(samplingSphere.getSphere().distance(listNode->getBB().getMin()) < 1.0e-9);
			assert(samplingSphere.getSphere().distance(listNode->getBB().getMax()) < 1.0e-9);
		}
		{
			// left => only first box must be visible
			const auto vv = samplingSphere.queryValue(positions[0], MinSG::SVS::INTERPOLATION_NEAREST);
			assert(vv.getBenefits(boxNodes.front().get()) > 0);
			for(uint_fast32_t boxIndex = 1; boxIndex < count; ++boxIndex) {
				MinSG::GeometryNode * geoNode = boxNodes[boxIndex].get();
				assert(vv.getBenefits(geoNode) == 0);
			}
		}
		{
			// right => only last box inside the subtree must be visible
			const auto vv = samplingSphere.queryValue(positions[1], MinSG::SVS::INTERPOLATION_NEAREST);
			const uint32_t lastBoxIndex = listIndex + 2;
			assert(vv.getBenefits(boxNodes[lastBoxIndex].get()) > 0);
			for(uint_fast32_t boxIndex = 0; boxIndex < count; ++boxIndex) {
				if(boxIndex != lastBoxIndex) {
					MinSG::GeometryNode * geoNode = boxNodes[boxIndex].get();
					assert(vv.getBenefits(geoNode) == 0);
				}
			}
		}
		// top, back => all boxes in the subtree must be visible
		for(uint_fast32_t posIndex = 2; posIndex <= 3; ++posIndex) {
			const auto vv = samplingSphere.queryValue(positions[posIndex], MinSG::SVS::INTERPOLATION_NEAREST);
			const auto geoNodes = MinSG::collectNodes<MinSG::GeometryNode>(listNode);
			for(const auto & geoNode : geoNodes) {
				assert(vv.getBenefits(geoNode) > 0);
			}
		}
	}

	boxNodes.clear();
	MinSG::destroy(listNodes.back().get());
	listNodes.clear();

	timer.stop();
	std::cout << "done (duration: " << timer.getSeconds() << " s).\n";
#endif /* MINSG_EXT_SVS */
	return EXIT_SUCCESS;
}
