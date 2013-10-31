/*
	This file is part of the MinSG library.
	Copyright (C) 2010-2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include <MinSG/Core/Nodes/CameraNode.h>
#include <MinSG/Core/Nodes/GeometryNode.h>
#include <MinSG/Core/Nodes/ListNode.h>
#include <MinSG/Core/FrameContext.h>

#include <MinSG/Core/Nodes/LightNode.h>
#include <MinSG/Core/States/LightingState.h>
#include <MinSG/Core/States/ShaderState.h>

#include <MinSG/Helper/Helper.h>

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/MeshUtils/PlatonicSolids.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Helper.h>

#include <Util/UI/EventContext.h>
#include <Util/UI/UI.h>
#include <Util/UI/Window.h>
#include <Util/Timer.h>
#include <Util/Util.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

static const std::string vertexProgram(
R"***(#version 110
uniform mat4 sg_modelViewProjectionMatrix;

attribute vec4 sg_Position;

void main() {
	gl_Position = sg_modelViewProjectionMatrix * sg_Position;
}
)***");
static const std::string fragmentProgram(
R"***(#version 110
uniform vec4 GlobalColor;

void main() {
	gl_FragColor = GlobalColor;
}
)***");

int main(int argc, char ** argv) {
	long maxSubdivisions = 10;
	if(argc > 1) {
		maxSubdivisions = std::strtol(argv[1], nullptr, 10);
	}

	Util::init();
	Util::UI::Window::Properties properties;
	properties.positioned = true;
	properties.posX = 0;
	properties.posY = 0;
	properties.clientAreaWidth = 300;
	properties.clientAreaHeight = 300;
	properties.title = "OpenGL Triangle Throughput Test";
	properties.compatibilityProfile = true;
	std::unique_ptr<Util::UI::Window> window(Util::UI::createWindow(properties));
	if(!window) {
		return EXIT_FAILURE;
	}

	Util::UI::EventContext eventContext;

	Rendering::RenderingContext::initGLState();
	eventContext.getEventQueue().registerEventGenerator(std::bind(&Util::UI::Window::fetchEvents, window.get()));

	Util::Reference<MinSG::GroupNode> root = new MinSG::ListNode;

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
		root->addChild(light);
		root->addState(state);
	}

	MinSG::FrameContext context;

	GET_GL_ERROR()

	{
		Rendering::Shader * shader = Rendering::Shader::createShader(vertexProgram,
																	 fragmentProgram,
																	 Rendering::Shader::USE_UNIFORMS);
		MinSG::ShaderState * shaderState = new MinSG::ShaderState(shader);
		shaderState->setUniform(context,
								Rendering::Uniform("GlobalColor", Util::Color4f(1.0f, 1.0f, 1.0f, 1.0f)));
		root->addState(shaderState);
	}

	{
		MinSG::CameraNode * camera = new MinSG::CameraNode;
		camera->setViewport(Geometry::Rect_i(0, 0, 300, 300));
		camera->setNearFar(0.1f, 1000.0f);
		camera->applyVerticalAngle(90.0f);
		camera->setRelPosition(Geometry::Vec3(0.0f, 0.0f, 5.0f));
		context.setCamera(camera);
	}

	std::cout << "Mesh size (triangles)\t" 
				<< "Mesh size (vertices)\t" 
				<< "Minimum throughput (million triangles per second)\t"
				<< "Median throughput (million triangles per second)\t"
				<< "Maximum throughput (million triangles per second)\t"
				<< "Minimum throughput (million vertices per second)\t"
				<< "Median throughput (million vertices per second)\t"
				<< "Maximum throughput (million vertices per second)"
				<< std::endl;

	Util::Reference<MinSG::GeometryNode> geoNode = new MinSG::GeometryNode;
	root->addChild(geoNode.get());
	for (uint_fast8_t i = 0; i < maxSubdivisions; ++i) {
		eventContext.getEventQueue().process();

		Util::Reference<Rendering::Mesh> icosahedron = Rendering::MeshUtils::PlatonicSolids::createIcosahedron();
		Util::Reference<Rendering::Mesh> sphere = Rendering::MeshUtils::PlatonicSolids::createEdgeSubdivisionSphere(icosahedron.get(), i);
		geoNode->setMesh(sphere.get());

		const auto numTriangles = sphere->getPrimitiveCount();
		const auto numVertices = sphere->getVertexCount();

		// Make sure the data is uploaded to graphics memory.
		root->display(context, 0);

		Util::Timer measure;
		uint16_t numRuns = 20;
		if (i < 6) {
			numRuns = 200;
		}
		std::vector<double> runningTimeSeconds;
		runningTimeSeconds.reserve(numRuns);
		for (uint_fast16_t run = 0; run < numRuns; ++run) {
			context.getRenderingContext().clearScreen(Util::Color4f(0.0f, 0.0f, 0.0f, 1.0f));

			context.getRenderingContext().finish();
			measure.reset();
			root->display(context, 0);
			context.getRenderingContext().finish();
			measure.stop();

			runningTimeSeconds.push_back(measure.getSeconds());

			window->swapBuffers();
		}
		std::sort(runningTimeSeconds.begin(), runningTimeSeconds.end());
		std::cout << numTriangles
					<< '\t' << numVertices
					<< '\t' << (numTriangles / runningTimeSeconds.back()) * 1.0e-6
					<< '\t' << (numTriangles / runningTimeSeconds[runningTimeSeconds.size() / 2]) * 1.0e-6
					<< '\t' << (numTriangles / runningTimeSeconds.front()) * 1.0e-6
					<< '\t' << (numVertices / runningTimeSeconds.back()) * 1.0e-6
					<< '\t' << (numVertices / runningTimeSeconds[runningTimeSeconds.size() / 2]) * 1.0e-6
					<< '\t' << (numVertices / runningTimeSeconds.front()) * 1.0e-6
					<< std::endl;
	}

	MinSG::destroy(root.get());
	geoNode = nullptr;
	root = nullptr;
	return EXIT_SUCCESS;
}
