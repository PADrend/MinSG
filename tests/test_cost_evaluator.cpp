/*
	This file is part of the MinSG library.
	Copyright (C) 2012-2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include <Geometry/Box.h>
#include <Geometry/Definitions.h>
#include <Geometry/Rect.h>
#include <Geometry/SRT.h>
#include <Geometry/Vec3.h>
#include <MinSG/Core/Nodes/CameraNodeOrtho.h>
#include <MinSG/Core/Nodes/GeometryNode.h>
#include <MinSG/Core/Nodes/ListNode.h>
#include <MinSG/Core/FrameContext.h>
#include <MinSG/Ext/VisibilitySubdivision/CostEvaluator.h>
#include <MinSG/Ext/VisibilitySubdivision/VisibilityVector.h>
#include <MinSG/Helper/Helper.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/MeshUtils/MeshBuilder.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Util/Graphics/Color.h>
#include <Util/UI/Window.h>
#include <Util/References.h>
#include <Util/Timer.h>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <vector>

// Prevent warning
int test_cost_evaluator(Util::UI::Window *);

int test_cost_evaluator(Util::UI::Window * window) {
#ifdef MINSG_EXT_VISIBILITY_SUBDIVISION
	std::cout << "Test CostEvaluator ... ";
	Util::Timer timer;
	timer.reset();

	MinSG::FrameContext frameContext;
	Util::Reference<MinSG::ListNode> scene = new MinSG::ListNode;

	Rendering::VertexDescription vertexDesc;
	vertexDesc.appendPosition3D();
	Util::Reference<Rendering::Mesh> boxMesh = Rendering::MeshUtils::MeshBuilder::createBox(vertexDesc, Geometry::Box(Geometry::Vec3f(0.5f, 0.5f, 0.5f), 1));

	/*
	 * Create a 2D array of n * n unit-size boxes in the z=0 plane
	 *
	 * ---------------- y=n
	 * |  |  |  |  |  |
	 * ----------------              -----------(b)  for all x, y in [0, n-1]:
	 * |  |  |  |  |  |              |           |
	 * ----------------              |           |   a = (x, y)
	 * |  |  |  |  |  |              |    (c)    |   b = (x + 1, y + 1)
	 * ----------------              |           |   c = (x + 0.5, y + 0.5)
	 * |  |  |  |  |  |              |           |
	 * ----------------             (a)-----------
	 * |  |  |  |  |  |
	 * ---------------- y=0
	 * x=0            x=n
	 */
	const int size = std::min(window->getWidth(), window->getHeight());
	assert(size % 4 == 0); // required to get same amount of pixels for all boxes
	const uint32_t count = static_cast<uint32_t>(size) / 4;
	const float countHalf = static_cast<float>(count) / 2.0f;

	std::vector<Util::Reference<MinSG::GeometryNode>> nodes;
	nodes.reserve(count * count);
	for(uint_fast32_t x = 0; x < count; ++x) {
		for(uint_fast32_t y = 0; y < count; ++y) {
			MinSG::GeometryNode * geoNode = new MinSG::GeometryNode(boxMesh);
			geoNode->moveLocal(Geometry::Vec3f(x, y, 0));
			nodes.push_back(geoNode);
			scene->addChild(geoNode);
		}
	}

	// Add inactive nodes to the left of the array that would block the line of sight.
	// This checks if the CostEvaluator ignores inactive nodes.
	for(uint_fast32_t y = 0; y < count; ++y) {
		MinSG::GeometryNode * geoNode = new MinSG::GeometryNode(boxMesh);
		geoNode->moveLocal(Geometry::Vec3f(-0.1f, y, 0));
		scene->addChild(geoNode);
		geoNode->deactivate();
	}

	Util::Reference<MinSG::CameraNodeOrtho> camera = new MinSG::CameraNodeOrtho;
	camera->setViewport(Geometry::Rect_i(0, 0, size, size), true);
	camera->setNearFar(0.5f, count + 2.0f);
	camera->setClippingPlanes(-countHalf, countHalf, -countHalf, countHalf);

	std::vector<Geometry::SRTf> cameraSrts(6);
	cameraSrts[static_cast<size_t>(Geometry::side_t::X_POS)] = Geometry::SRTf(Geometry::Vec3f(count + 1.0f, countHalf, countHalf), Geometry::Vec3f(1, 0, 0), Geometry::Vec3f(0, 1, 0));
	cameraSrts[static_cast<size_t>(Geometry::side_t::X_NEG)] = Geometry::SRTf(Geometry::Vec3f(-1.0f, countHalf, countHalf), Geometry::Vec3f(-1, 0, 0), Geometry::Vec3f(0, 1, 0));
	cameraSrts[static_cast<size_t>(Geometry::side_t::Y_POS)] = Geometry::SRTf(Geometry::Vec3f(countHalf, count + 1.0f, countHalf), Geometry::Vec3f(0, 1, 0), Geometry::Vec3f(0, 0, 1));
	cameraSrts[static_cast<size_t>(Geometry::side_t::Y_NEG)] = Geometry::SRTf(Geometry::Vec3f(countHalf, -1.0f, countHalf), Geometry::Vec3f(0, -1, 0), Geometry::Vec3f(0, 0, 1));
	cameraSrts[static_cast<size_t>(Geometry::side_t::Z_POS)] = Geometry::SRTf(Geometry::Vec3f(countHalf, countHalf, count + 1.0f), Geometry::Vec3f(0, 0, 1), Geometry::Vec3f(0, 1, 0));
	cameraSrts[static_cast<size_t>(Geometry::side_t::Z_NEG)] = Geometry::SRTf(Geometry::Vec3f(countHalf, countHalf, -1.0f), Geometry::Vec3f(0, 0, -1), Geometry::Vec3f(0, 1, 0));

	std::vector<MinSG::VisibilitySubdivision::VisibilityVector> results;
	results.reserve(6);

	// Measurements
	for(const auto & cameraSrt : cameraSrts) {
		camera->setRelTransformation(cameraSrt);
		frameContext.setCamera(camera.get());
		frameContext.getRenderingContext().clearScreen(Util::Color4f(0.5f, 0.5f, 0.5f, 1.0f));

		MinSG::VisibilitySubdivision::CostEvaluator evaluator(MinSG::Evaluators::Evaluator::SINGLE_VALUE);
		evaluator.beginMeasure();
		evaluator.measure(frameContext, *scene.get(), Geometry::Rect_f(camera->getViewport()));
		evaluator.endMeasure(frameContext);

		const Util::GenericAttributeList * resultList = evaluator.getResults();
		assert(resultList != nullptr);
		const Util::GenericAttribute * firstResult = resultList->front();
		assert(firstResult != nullptr);
		auto vva = dynamic_cast<const MinSG::VisibilitySubdivision::VisibilityVectorAttribute *>(firstResult);
		assert(vva != nullptr);
		results.emplace_back(vva->ref());
	}

	// Because we divide the screen size by four above for calculating count, every box has to be four times four pixels.
	const MinSG::VisibilitySubdivision::VisibilityVector::benefits_t numPixels = 4 * 4;

	// Validate the results
	for(uint_fast32_t x = 0; x < count; ++x) {
		for(uint_fast32_t y = 0; y < count; ++y) {
			MinSG::GeometryNode * geoNode = nodes[x * count + y].get();
			const auto leftBenefits = results[static_cast<size_t>(Geometry::side_t::X_NEG)].getBenefits(geoNode);
			assert(leftBenefits == ((x == 0) ? numPixels : 0)); // left => only first column visible
			const auto bottomBenefits = results[static_cast<size_t>(Geometry::side_t::Y_NEG)].getBenefits(geoNode);
			assert(bottomBenefits == ((y == 0) ? numPixels : 0)); // bottom => only first row visible
			const auto backBenefits = results[static_cast<size_t>(Geometry::side_t::Z_NEG)].getBenefits(geoNode);
			assert(backBenefits == numPixels); // back => everything visible
			const auto rightBenefits = results[static_cast<size_t>(Geometry::side_t::X_POS)].getBenefits(geoNode);
			assert(rightBenefits == ((x == count - 1) ? numPixels : 0)); // right => only last column visible
			const auto topBenefits = results[static_cast<size_t>(Geometry::side_t::Y_POS)].getBenefits(geoNode);
			assert(topBenefits == ((y == count - 1) ? numPixels : 0)); // top => only last row visible
			const auto frontBenefits = results[static_cast<size_t>(Geometry::side_t::Z_POS)].getBenefits(geoNode);
			assert(frontBenefits == numPixels); // front => everything visible
		}
	}

	results.clear();
	camera = nullptr;
	nodes.clear();
	MinSG::destroy(scene.get());
	scene = nullptr;

	timer.stop();
	std::cout << "done (duration: " << timer.getSeconds() << " s).\n";
#endif /* MINSG_EXT_VISIBILITY_SUBDIVISION */
	return EXIT_SUCCESS;
}
