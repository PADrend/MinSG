/*
	This file is part of the MinSG library.
	Copyright (C) 2010-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2009-2012 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "Viewer/MoveNodeHandler.h"

#include <Geometry/Frustum.h>

#include <MinSG/Core/Nodes/CameraNode.h>
#include <MinSG/Core/Nodes/GeometryNode.h>
#include <MinSG/Core/Nodes/ListNode.h>
#include <MinSG/Core/States/LightingState.h>
#include <MinSG/Core/States/TextureState.h>
#include <MinSG/Core/Behaviours/BehaviourManager.h>
#include <MinSG/Core/FrameContext.h>
#include <MinSG/Ext/Nodes/GenericMetaNode.h>
#include <MinSG/Ext/States/SkyboxState.h>
#include <MinSG/Helper/Helper.h>

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/MeshUtils/MeshUtils.h>
#include <Rendering/MeshUtils/PlatonicSolids.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Serialization/Serialization.h>
#include <Rendering/Helper.h>

#include <Util/UI/Event.h>
#include <Util/UI/EventContext.h>
#include <Util/UI/EventQueue.h>
#include <Util/UI/Window.h>
#include <Util/References.h>
#include <Util/StringUtils.h>
#include <Util/Timer.h>

#include <iostream>
#include <random>

using namespace MinSG;

// Prevent warning
int test_large_scene(Util::UI::Window * window, Util::UI::EventContext & eventContext);

int test_large_scene(Util::UI::Window * window, Util::UI::EventContext & eventContext) {
	// texture registry
	std::map<std::string, Util::Reference<Rendering::Texture> > textures;

	std::cout << "Create FrameContext...\n";
	FrameContext fc;

	unsigned int renderingFlags = /*BOUNDING_BOXES|SHOW_META_OBJECTS|*/FRUSTUM_CULLING/*|SHOW_COORD_SYSTEM*/;//|SHOW_COORD_SYSTEM;


	std::cout << "Create scene graph...\n";
	Util::Reference<GroupNode> root = new MinSG::ListNode();


	/// Skybox
	SkyboxState * sb = SkyboxState::createSkybox("Data/texture/?.bmp");
	root->addState(sb);


	/// Some shperes...
	{
		std::default_random_engine engine;
		std::uniform_real_distribution<float> coordinateDist(0.0f, 200.0f);
		
		std::vector<Util::Reference<Rendering::Mesh> > spheres;
		Util::Reference<Rendering::Mesh> icosahedron = Rendering::MeshUtils::PlatonicSolids::createIcosahedron();
		for(int i=0;i<6;++i)
			spheres.push_back(Rendering::MeshUtils::PlatonicSolids::createEdgeSubdivisionSphere(icosahedron.get(), i)); // 6... 81920 triangles each

		for (int i = 0; i < 1000; i++) {
			// create a real clone inclusive internal data!
			MinSG::GeometryNode * gn = new GeometryNode(spheres[std::uniform_int_distribution<std::size_t>(0, spheres.size() - 1)(engine)]->clone());
			gn->moveRel(Geometry::Vec3(coordinateDist(engine), coordinateDist(engine), coordinateDist(engine)));
			root->addChild(gn);
			gn->scale(0.1 + std::uniform_real_distribution<float>(0.0f, 1000.0f)(engine) / 400.0);
		}
	}

	/// Camera
	Node * schwein = loadModel(Util::FileName("Data/model/Schwein.low.t.ply"), MESH_AUTO_CENTER | MESH_AUTO_SCALE);
	schwein->addState( loadTexture(Util::FileName("Data/texture/schwein.low.t.bmp")));

	ListNode * camera = new ListNode();
	CameraNode * camNode = new CameraNode();
	camNode->setViewport(Geometry::Rect_i(0, 0, 1024, 768));
	camNode->setNearFar(0.1, 2000);
	camNode->applyVerticalAngle(80);
	camNode->moveRel(Geometry::Vec3(0, 4, 10));
	camera->addChild(camNode);
	camera->addChild(schwein);
	schwein->moveRel(Geometry::Vec3(0, 0, 0));
	schwein->rotateLocal_deg(180, Geometry::Vec3(0, 1, 0));

	LightNode * myHeadLight = LightNode::createPointLight();
	myHeadLight->scale(1);
	myHeadLight->moveRel(Geometry::Vec3(0, 0, 0));
	camera->addChild(myHeadLight);
	LightingState * lightState = new LightingState;
	lightState->setLight(myHeadLight);
	root->addState(lightState);

	root->addChild(camera);


	/// Eventhandler
	MoveNodeHandler * eh = new MoveNodeHandler();
	MoveNodeHandler::initClaudius(eh, camera);


	// ---------------------------------------------------------------------------------------------

	Rendering::RenderingContext::clearScreen(Util::Color4f(0.5f, 0.5f, 0.5f, 0.5f));

	// ----
	GET_GL_ERROR();

	uint32_t fpsFrameCounter = 0;
	Util::Timer fpsTimer;

	std::cout << "\nEntering main loop...\n";


	// program main loop
	bool done = false;
	while (!done) {
		++fpsFrameCounter;
		double seconds = fpsTimer.getSeconds();
		if (seconds > 1.0) {
			double fps = static_cast<double> (fpsFrameCounter) / seconds;
			std::cout << "\r " << fps << " fps    ";
			std::cout.flush();
			fpsTimer.reset();
			fpsFrameCounter = 0;
		}

		// message processing loop
		eventContext.getEventQueue().process();
		while (eventContext.getEventQueue().getNumEventsAvailable() > 0) {
			Util::UI::Event event = eventContext.getEventQueue().popEvent();
			// check for messages
			switch (event.type) {
				// exit if the window is closed
				case Util::UI::EVENT_QUIT:
					done = true;
					break;


					// check for keypresses
				case Util::UI::EVENT_KEYBOARD: {
					if(event.keyboard.pressed && event.keyboard.key == Util::UI::KEY_ESCAPE) {
						done = true;
					}
					break;
				}

			} // end switch
		} // end of message processing

		// apply translation
		eh->execute();


		// clear screen
		Rendering::RenderingContext::clearScreen(Util::Color4f(0.0f, 0.0f, 0.0f, 1.0f));


		// enable Camera
		fc.setCamera(camNode);


		// render Scene
		root->display(fc, renderingFlags);

		window->swapBuffers();
		GET_GL_ERROR();
	} // end main loop


	// destroy scene graph
	MinSG::destroy(root.get());
	root = nullptr;

	// all is well ;)
	std::cout << "Exited cleanly\n";
	//system("pause");
	return EXIT_SUCCESS;
}
