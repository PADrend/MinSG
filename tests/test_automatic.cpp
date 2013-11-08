/*
	This file is part of the MinSG library.
	Copyright (C) 2010-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2010-2012 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "Viewer/MoveNodeHandler.h"

#include <MinSG/Core/Nodes/CameraNode.h>
#include <MinSG/Core/Nodes/GeometryNode.h>
#include <MinSG/Core/Nodes/LightNode.h>
#include <MinSG/Core/Nodes/ListNode.h>
#include <MinSG/Core/States/LightingState.h>
#include <MinSG/Core/States/TextureState.h>
#include <MinSG/Ext/States/SkyboxState.h>
#include <MinSG/Helper/Helper.h>

#include <Geometry/Vec3.h>

#include <Rendering/Texture/TextureUtils.h>

#include <Util/Timer.h>

#include <cstdlib>
#include <iostream>
#include <random>

using namespace MinSG;

// Prevent warning
int test_automatic();

int test_automatic() {
	std::cout << "### MinSG tests ###\n";
	{
		std::cout << "Test ListNode ... ";


		// Creation
		Util::Reference<ListNode> ln = new ListNode;
		if (ln->countChildren() != 0) {
			std::cout << "ListNode must not have any children." << std::endl;
			return EXIT_FAILURE;
		}

		// Insertion
		ListNode * a = new ListNode;
		ListNode * b = new ListNode;
		ListNode * c = new ListNode;

		ln->addChild(a);
		if (ln->countChildren() != 1) {
			std::cout << "ListNode must have one child." << std::endl;
			return EXIT_FAILURE;
		}
		ln->addChild(b);
		if (ln->countChildren() != 2) {
			std::cout << "ListNode must have two children." << std::endl;
			return EXIT_FAILURE;
		}
		ln->addChild(c);
		if (ln->countChildren() != 3) {
			std::cout << "ListNode must have three children." << std::endl;
			return EXIT_FAILURE;
		}

		// Tree updates
		Geometry::SRT srtA, srtB, srtC;

		srtA.setTranslation(Geometry::Vec3(5.0f, 0.0f, 0.0f));
		srtB.setTranslation(Geometry::Vec3(0.0f, 5.0f, 0.0f));
		srtC.setTranslation(Geometry::Vec3(0.0f, 0.0f, 5.0f));

		{
			const Geometry::Box & listBB = ln->getWorldBB();
			if (listBB.contains(srtA.getTranslation()) || listBB.contains(srtB.getTranslation()) || listBB.contains(srtC.getTranslation())) {
				std::cout << "ListNode has invalid absolute bounding box." << std::endl;
				return EXIT_FAILURE;
			}
		}

		a->setSRT(srtA);
		{
			const Geometry::Box & listBB = ln->getWorldBB();
			if (!listBB.contains(srtA.getTranslation()) || listBB.contains(srtB.getTranslation()) || listBB.contains(srtC.getTranslation())) {
				std::cout << "ListNode has invalid absolute bounding box." << std::endl;
				return EXIT_FAILURE;
			}
		}

		b->setSRT(srtB);
		{
			const Geometry::Box & listBB = ln->getWorldBB();
			if (!listBB.contains(srtA.getTranslation()) || !listBB.contains(srtB.getTranslation()) || listBB.contains(srtC.getTranslation())) {
				std::cout << "ListNode has invalid absolute bounding box." << std::endl;
				return EXIT_FAILURE;
			}
		}

		c->setSRT(srtC);
		{
			const Geometry::Box & listBB = ln->getWorldBB();
			if (!listBB.contains(srtA.getTranslation()) || !listBB.contains(srtB.getTranslation()) || !listBB.contains(srtC.getTranslation())) {
				std::cout << "ListNode has invalid absolute bounding box." << std::endl;
				return EXIT_FAILURE;
			}
		}

		// Deletion
		ln->removeChild(a);
		if (ln->countChildren() != 2) {
			std::cout << "ListNode must have two children." << std::endl;
			return EXIT_FAILURE;
		}
		ln->removeChild(b);
		if (ln->countChildren() != 1) {
			std::cout << "ListNode must have one child." << std::endl;
			return EXIT_FAILURE;
		}
		ln->removeChild(c);
		if (ln->countChildren() != 0) {
			std::cout << "ListNode must not have any children." << std::endl;
			return EXIT_FAILURE;
		}

		// Destruction
		ln = nullptr;

		std::cout << "done.\n";
	}

	std::cout << "Create chess texture ... ";
	Rendering::Texture * t = Rendering::TextureUtils::createChessTexture(64, 64);
	std::cout << "done.\n";

	std::cout << "Create scene graph root (ListNode) ... ";
	Util::Reference<GroupNode> root = new ListNode();
	std::cout << "done.\n";

	std::cout << "Add empty GeometryNode ... ";
	GeometryNode * geo = new GeometryNode;
	root->addChild(geo);
	std::cout << "done.\n";

	std::cout << "Attach TextureNode ... ";
	TextureState * tn = new TextureState(t);
	geo->addState(tn);
	std::cout << "done.\n";

	std::default_random_engine engine;
	std::uniform_real_distribution<float> coordinateDist(0.0f, 200.0f);

	std::cout << "Clone GeometryNode, move and scale ... ";
	for (uint_fast16_t i = 0; i < 1000; ++i) {
		GeometryNode * b2 = dynamic_cast<GeometryNode*> (geo->clone());
		root->addChild(b2);
		b2->moveRel(Geometry::Vec3(coordinateDist(engine), coordinateDist(engine), coordinateDist(engine)));
		b2->scale(0.1 + std::uniform_real_distribution<float>(0.0f, 1000.0f)(engine) / 400.0);
	}
	std::cout << "done.\n";

	std::cout << "Add CameraNode ... ";
	CameraNode * camera = new CameraNode();
	camera->setViewport(Geometry::Rect_i(0, 0, 1024, 768));
	camera->setNearFar(0.1, 2000);
	camera->applyVerticalAngle(80);
	camera->moveRel(Geometry::Vec3(0, 4, 10));
	root->addChild(camera);
	std::cout << "done.\n";

	std::cout << "Add LightNodeOmni ... ";
	LightNode * light = LightNode::createPointLight();
	light->scale(1);
	light->moveRel(Geometry::Vec3(0, 0, 0));
	LightingState * lightState = new LightingState;
	lightState->setLight(light);
	root->addState(lightState);
	root->addChild(light);
	std::cout << "done.\n";

	std::cout << "Create MoveNodeHandler ... ";
	MoveNodeHandler * eh = new MoveNodeHandler();
	MoveNodeHandler::initClaudius(eh, camera);
	std::cout << "done.\n";

	std::cout << "Destroy scene graph ... ";
	MinSG::destroy(root.get());
	root = nullptr;
	std::cout << "done.\n";

	return EXIT_SUCCESS;
}
