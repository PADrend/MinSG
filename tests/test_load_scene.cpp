/*
	This file is part of the MinSG library.
	Copyright (C) 2011-2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "Viewer/MoveNodeHandler.h"

#include <MinSG/Core/Nodes/CameraNode.h>
#include <MinSG/Core/Nodes/LightNode.h>
#include <MinSG/Core/Nodes/ListNode.h>
#include <MinSG/Core/Nodes/Node.h>
#include <MinSG/Core/States/LightingState.h>
#include <MinSG/Core/FrameContext.h>
#include <MinSG/Core/RenderParam.h>
#include <MinSG/Helper/Helper.h>
#include <MinSG/SceneManagement/ImportFunctions.h>
#include <MinSG/SceneManagement/SceneManager.h>
#include <Rendering/Helper.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Util/UI/Event.h>
#include <Util/UI/EventContext.h>
#include <Util/UI/EventQueue.h>
#include <Util/UI/Window.h>
#include <Util/IO/FileName.h>
#include <Util/Graphics/Color.h>
#include <Util/References.h>
#include <Util/Timer.h>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <memory>
#include <stdint.h>
#include <string>

// Prevent warning
int test_load_scene(Util::UI::Window * window, Util::UI::EventContext & eventContext);

int test_load_scene(Util::UI::Window * window, Util::UI::EventContext & eventContext) {
	// Create scene graph
	Util::Reference<MinSG::ListNode> root = new MinSG::ListNode();

	MinSG::ListNode * dolly = new MinSG::ListNode();
	root->addChild(dolly);

	MinSG::CameraNode * camera = new MinSG::CameraNode();
	camera->setViewport(Geometry::Rect_i(0, 0, 1024, 768));
	camera->setNearFar(1, 1000);
	camera->applyVerticalAngle(60);
	dolly->addChild(camera);

	MinSG::LightNode * myHeadLight = MinSG::LightNode::createPointLight();
	dolly->addChild(myHeadLight);

	MinSG::LightingState * lightState = new MinSG::LightingState();
	lightState->setLight(myHeadLight);
	root->addState(lightState);

	// Set up MinSG objects
	MinSG::FrameContext frameContext;
	MinSG::SceneManagement::SceneManager sceneManager;

	// Load a scene
	{
		const Util::FileName sceneFile("test-scene.minsg");
		std::cout << "Loading scene \"" << sceneFile.toString() << "\"." << std::endl;
		const auto nodes = MinSG::SceneManagement::loadMinSGFile(sceneManager, sceneFile);
		std::cout << "Loaded " << nodes.size() << " node(s)." << std::endl;
		for(const auto & node : nodes) {
			root->addChild(node.get());
		}
	}

	// Set up user movement
	MinSG::MoveNodeHandler * moveNodeHandler = new MinSG::MoveNodeHandler();
	MinSG::MoveNodeHandler::initClaudius(moveNodeHandler, dolly);

	// Set up event loop
	uint32_t fpsFrameCounter = 0;
	Util::Timer fpsTimer;

	bool done = false;
	while (!done) {
		// FPS calculation
		++fpsFrameCounter;
		const double seconds = fpsTimer.getSeconds();
		if (seconds > 1.0) {
			const double fps = static_cast<double>(fpsFrameCounter) / seconds;
			std::cout << fps << " fps" << std::endl;
			fpsTimer.reset();
			fpsFrameCounter = 0;
		}

		// Event handling
		eventContext.getEventQueue().process();
		while (eventContext.getEventQueue().getNumEventsAvailable() > 0) {
			const Util::UI::Event event = eventContext.getEventQueue().popEvent();
			if (moveNodeHandler->process(event)) {
				continue;
			}
			switch (event.type) {
				case Util::UI::EVENT_QUIT:
					done = true;
					break;
				case Util::UI::EVENT_KEYBOARD:
					if(event.keyboard.pressed && event.keyboard.key == Util::UI::KEY_ESCAPE) {
						done = true;
					}
					break;
				default:
					break;
			}
		}
		moveNodeHandler->execute();

		// Rendering
		frameContext.beginFrame();
		frameContext.getRenderingContext().clearScreen(Util::Color4f(0.0f, 0.0f, 0.0f, 1.0f));
		frameContext.setCamera(camera);
		root->display(frameContext, MinSG::FRUSTUM_CULLING);
		frameContext.endFrame();

		window->swapBuffers();
		GET_GL_ERROR();
	}

	MinSG::destroy(root.get());
	root = nullptr;

	return EXIT_SUCCESS;
}
