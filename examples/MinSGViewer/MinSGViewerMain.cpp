/*
	This file is part of the MinSG library.
	Copyright (C) 2010-2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include <MinSG/Core/Nodes/CameraNode.h>
#include <MinSG/Core/Nodes/ListNode.h>
#include <MinSG/Core/FrameContext.h>

#include <MinSG/Core/Nodes/LightNode.h>
#include <MinSG/Core/States/LightingState.h>

#include <MinSG/Helper/DataDirectory.h>
#include <MinSG/Helper/Helper.h>

#include <MinSG/SceneManagement/ImportFunctions.h>
#include <MinSG/SceneManagement/SceneManager.h>

#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Helper.h>

#include <Util/IO/FileName.h>
#include <Util/IO/FileUtils.h>
#include <Util/UI/EventQueue.h>
#include <Util/UI/Event.h>
#include <Util/UI/UI.h>
#include <Util/UI/Window.h>
#include <Util/Timer.h>
#include <Util/Util.h>

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>

class Application {
	private:
		bool running;
		Util::Reference<MinSG::CameraNode> camera;
		Util::Reference<MinSG::ListNode> scene;
		MinSG::SceneManagement::SceneManager sceneMgr;

	public:
		Application() :
			running(true),
			camera(new MinSG::CameraNode),
			scene(new MinSG::ListNode),
			sceneMgr() {
		}

		bool processEvent(const Util::UI::Event & event) {
			switch (event.type) {
				case Util::UI::EVENT_QUIT:
					running = false;
					break;
				case Util::UI::EVENT_RESIZE:
					camera->setViewport(Geometry::Rect_i(0, 0, event.resize.width, event.resize.height));
					camera->applyVerticalAngle(60.0f);
					break;
				case Util::UI::EVENT_KEYBOARD:
					if (event.keyboard.pressed) {
						const float stepSize = 1.0f;
						switch (event.keyboard.key) {
							case Util::UI::KEY_W:
								camera->moveLocal(Geometry::Vec3(0.0f, 0.0f, -stepSize));
								break;
							case Util::UI::KEY_S:
								camera->moveLocal(Geometry::Vec3(0.0f, 0.0f, stepSize));
								break;
							case Util::UI::KEY_A:
								camera->moveLocal(Geometry::Vec3(-stepSize, 0.0f, 0.0f));
								break;
							case Util::UI::KEY_D:
								camera->moveLocal(Geometry::Vec3(stepSize, 0.0f, 0.0f));
								break;
							case Util::UI::KEY_R:
								camera->moveLocal(Geometry::Vec3(0.0f, stepSize, 0.0f));
								break;
							case Util::UI::KEY_F:
								camera->moveLocal(Geometry::Vec3(0.0f, -stepSize, 0.0f));
								break;
							case Util::UI::KEY_E:
								camera->rotateRel_deg(-10.0f, Geometry::Vec3(0.0f, 1.0f, 0.0f));
								break;
							case Util::UI::KEY_Q:
								camera->rotateRel_deg(10.0f, Geometry::Vec3(0.0f, 1.0f, 0.0f));
								break;
							case Util::UI::KEY_ESCAPE:
								running = false;
								break;
							default:
								break;
						}
					}
					break;
				default:
					break;
			}
			return true;
		}

		void loadScene(const Util::FileName & sceneFile) {
			if(Util::FileUtils::isFile(sceneFile)) {
				std::cout << "Loading scene from \"" << sceneFile << '\"' << std::endl;
				const auto nodes = MinSG::SceneManagement::loadMinSGFile(sceneMgr, sceneFile);
				if(nodes.empty()) {
					std::cerr << "Error: Empty scene file." << std::endl;
				} else if(nodes.size() == 1) {
					scene = dynamic_cast<MinSG::ListNode *>(nodes.front().get());
				} else {
					scene = new MinSG::ListNode;
					for(const auto & node : nodes) {
						scene->addChild(node.get());
					}
				}
			}
		}

		int mainLoop() {
			Util::UI::Window::Properties properties;
			properties.positioned = false;
			properties.clientAreaWidth = 1280;
			properties.clientAreaHeight = 720;
			properties.title = "MinSGViewer";
			properties.compatibilityProfile = true;
			std::unique_ptr<Util::UI::Window> window(Util::UI::createWindow(properties));
			if(!window) {
				return EXIT_FAILURE;
			}

			Util::UI::EventQueue eventQueue;
			eventQueue.registerEventGenerator(std::bind(&Util::UI::Window::fetchEvents, window.get()));
			eventQueue.registerEventHandler(std::bind(&Application::processEvent, this, std::placeholders::_1));

			Rendering::RenderingContext::initGLState();
			GET_GL_ERROR()
			Rendering::outputGLInformation(std::cout);

			camera->setViewport(Geometry::Rect_i(0, 0, 1280, 720));
			camera->setNearFar(0.1f, 1000.0f);
			camera->applyVerticalAngle(60.0f);

			{
				MinSG::LightNode * light = new MinSG::LightNode(Rendering::LightParameters::POINT);
				light->setConstantAttenuation(1.0f);
				light->setLinearAttenuation(0.0f);
				light->setQuadraticAttenuation(0.0f);
				light->setRelPosition(Geometry::Vec3(200.0f, 500.0f, 200.0f));
				light->setAmbientLightColor(Util::Color4f(0.2f, 0.2f, 0.2f, 1.0f));
				light->setAmbientLightColor(Util::Color4f(0.8f, 0.8f, 0.8f, 1.0f));
				light->setAmbientLightColor(Util::Color4f(1.0f, 1.0f, 1.0f, 1.0f));
				MinSG::LightingState * state = new MinSG::LightingState;
				state->setLight(light);
				scene->addChild(light);
				scene->addState(state);
			}

			GET_GL_ERROR()

			MinSG::FrameContext context;

			Util::Reference<Rendering::Shader> shader = Rendering::Shader::loadShader(Util::FileName(MinSG::DataDirectory::getPath() + "/shader/Phong.vs"),
																					  Util::FileName(MinSG::DataDirectory::getPath() + "/shader/Phong.fs"),
																					  Rendering::Shader::USE_UNIFORMS);
			context.getRenderingContext().pushAndSetShader(shader.get());

			Util::Timer fpsTimer;
			uint32_t fpsCounter = 0;
			while (running) {
				const auto seconds = fpsTimer.getSeconds();
				if (seconds > 3.0) {
					const auto fps = static_cast<double>(fpsCounter) / seconds;
					std::cout << fps << " fps" << std::endl;
					fpsTimer.reset();
					fpsCounter = 0;
				}
				++fpsCounter;

				eventQueue.process();

				context.getRenderingContext().clearScreen(Util::Color4f(0.5f, 0.5f, 0.5f, 1.0f));

				context.setCamera(camera.get());

				scene->display(context, 0);
				window->swapBuffers();
			}

			MinSG::destroy(scene.get());
			scene = nullptr;
			return EXIT_SUCCESS;
		}
};

int main(int argc, char ** argv) {
	if(argc != 2) {
		std::cout << "Usage: " << argv[0] << " [MinSG scene file]" << std::endl;
		return EXIT_SUCCESS;
	}
	Util::init();
	Application app;
	app.loadScene(Util::FileName(argv[1]));
	std::cout << "Press ...\n"
		<< "... [Esc] to quit.\n"
		<< "... [W], [A], [S], [D] to move forward, left, backward, right.\n"
		<< "... [R], [F] to move up, down.\n"
		<< "... [Q], [E] to turn left, right." << std::endl;
	return app.mainLoop();
}
