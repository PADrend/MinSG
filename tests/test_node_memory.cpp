/*
	This file is part of the MinSG library.
	Copyright (C) 2011-2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include <MinSG/Core/Nodes/GeometryNode.h>
#include <MinSG/Core/Nodes/ListNode.h>
#include <MinSG/Core/Nodes/Node.h>
#include <MinSG/Core/States/MaterialState.h>
#include <MinSG/Core/FrameContext.h>
#include <MinSG/Core/RenderParam.h>
#include <MinSG/Helper/Helper.h>
#include <MinSG/Helper/StdNodeVisitors.h>
#include <MinSG/SceneManagement/SceneManager.h>
#include <Geometry/Vec3.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/MeshUtils/MeshBuilder.h>
#include <Rendering/MeshUtils/PlatonicSolids.h>
#include <Rendering/Shader/Uniform.h>
#include <Util/GenericAttribute.h>
#include <Util/References.h>
#include <Util/StringUtils.h>
#include <Util/TypeNameMacro.h>
#include <Util/Utils.h>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <memory>
#include <string>
#include <stdint.h>

using namespace MinSG;

int test_node_memory() {
	MinSG::FrameContext fc;
	
	Util::Reference<Rendering::Mesh> icosahedron = Rendering::MeshUtils::PlatonicSolids::createIcosahedron();
	Util::Reference<Rendering::Mesh> sphere(Rendering::MeshUtils::PlatonicSolids::createEdgeSubdivisionSphere(icosahedron.get(), 2));
	uint32_t count = 100000;

	std::deque<Node*> nodes;
	double actual = 0, before = Util::Utils::getResidentSetMemorySize();

	actual = Util::Utils::getResidentSetMemorySize();
	std::cerr << Util::StringUtils::toFormattedString((actual - before) / count) << " before test\n";
	before = actual;

	while (nodes.size() < count/2){
		GeometryNode * gn = new GeometryNode();
		nodes.push_back(gn);
	}
	Util::Reference<ListNode> root;

	while (nodes.size() > 1)
	{
		root = new ListNode();
		root->addChild(nodes[0]);
		root->addChild(nodes[1]);
		nodes.pop_front();
		nodes.pop_front();
		nodes.push_back(root.get());
	}

	nodes = collectNodes<Node>(root.get());
	count = nodes.size();

	actual = Util::Utils::getResidentSetMemorySize();
	std::cerr << Util::StringUtils::toFormattedString((actual - before) / count) << " after creating nodes\n";
	before = actual;

	fc.displayNode(root.get(), 0);

	actual = Util::Utils::getResidentSetMemorySize();
	std::cerr << Util::StringUtils::toFormattedString((actual - before) / count) << " after display\n";
	before = actual;

	for(std::deque<Node*>::iterator it = nodes.begin(); it != nodes.end(); it++){
		GeometryNode * gn = dynamic_cast<GeometryNode*>(*it);
		if(gn)
			gn->setMesh(sphere->clone());
	}

	actual = Util::Utils::getResidentSetMemorySize();
	std::cerr << Util::StringUtils::toFormattedString((actual - before) / count) << " after setting meshes\n";
	before = actual;

	fc.displayNode(root.get(), 0);

	actual = Util::Utils::getResidentSetMemorySize();
	std::cerr << Util::StringUtils::toFormattedString((actual - before) / count) << " after display\n";
	before = actual;

	fc.displayNode(root.get(), USE_WORLD_MATRIX);

	actual = Util::Utils::getResidentSetMemorySize();
	std::cerr << Util::StringUtils::toFormattedString((actual - before) / count) << " after display world matrix\n";
	before = actual;

	for(std::deque<Node*>::iterator it = nodes.begin(); it != nodes.end(); it++){
		Node * n = *it;
		n->moveLocal(Geometry::Vec3f(1,1,1));
	}

	actual = Util::Utils::getResidentSetMemorySize();
	std::cerr << Util::StringUtils::toFormattedString((actual - before) / count) << " after moving nodes\n";
	before = actual;

	fc.displayNode(root.get(), 0);

	actual = Util::Utils::getResidentSetMemorySize();
	std::cerr << Util::StringUtils::toFormattedString((actual - before) / count) << " after display\n";
	before = actual;

	fc.displayNode(root.get(), USE_WORLD_MATRIX);

	actual = Util::Utils::getResidentSetMemorySize();
	std::cerr << Util::StringUtils::toFormattedString((actual - before) / count) << " after display world matrix\n";
	before = actual;

	for(std::deque<Node*>::iterator it = nodes.begin(); it != nodes.end(); it++){
		Node * n = *it;
		n->setAttribute(Util::StringIdentifier("ColorCube"), Util::GenericAttribute::createNumber(17));
	}

	actual = Util::Utils::getResidentSetMemorySize();
	std::cerr << Util::StringUtils::toFormattedString((actual - before) / count) << " after first attrib\n";
	before = actual;

	for(std::deque<Node*>::iterator it = nodes.begin(); it != nodes.end(); it++){
		Node * n = *it;
		n->setAttribute(Util::StringIdentifier("ColorCube2"), Util::GenericAttribute::createNumber(17));
	}

	actual = Util::Utils::getResidentSetMemorySize();
	std::cerr << Util::StringUtils::toFormattedString((actual - before) / count) << " after second attrib\n";
	before = actual;

	for(std::deque<Node*>::iterator it = nodes.begin(); it != nodes.end(); it++){
		Node * n = *it;
		n->setAttribute(Util::StringIdentifier("a"), Util::GenericAttribute::createNumber(17));
	}

	actual = Util::Utils::getResidentSetMemorySize();
	std::cerr << Util::StringUtils::toFormattedString((actual - before) / count) << " after third attrib\n";
	before = actual;

	for(std::deque<Node*>::iterator it = nodes.begin(); it != nodes.end(); it++){
		Node * n = *it;
		n->setAttribute(Util::StringIdentifier("ColorCube2ColorCube2ColorCube2ColorCube2ColorCube2ColorCube2ColorCube2ColorCube2"), Util::GenericAttribute::createNumber(17));
	}

	actual = Util::Utils::getResidentSetMemorySize();
	std::cerr << Util::StringUtils::toFormattedString((actual - before) / count) << " after fourth attrib\n";
	before = actual;

	for(std::deque<Node*>::iterator it = nodes.begin(); it != nodes.end(); it++){
		Node * n = *it;
		n->setAttribute(Util::StringIdentifier("b"), Util::GenericAttribute::createNumber(17));
	}

	actual = Util::Utils::getResidentSetMemorySize();
	std::cerr << Util::StringUtils::toFormattedString((actual - before) / count) << " after fifth attrib\n";
	before = actual;

	for(std::deque<Node*>::iterator it = nodes.begin(); it != nodes.end(); it++){
		Node * n = *it;
		n->addState(new MaterialState());
	}

	actual = Util::Utils::getResidentSetMemorySize();
	std::cerr << Util::StringUtils::toFormattedString((actual - before) / count) << " after add material\n";
	before = actual;

	SceneManagement::SceneManager sceneManager;
	
	int i=0;
	for(std::deque<Node*>::iterator it = nodes.begin(); it != nodes.end(); it++){
		Node * n = *it;
		sceneManager.registerNode(n->getClassName() + Util::StringUtils::toString(i++), n);
	}

	actual = Util::Utils::getResidentSetMemorySize();
	std::cerr << Util::StringUtils::toFormattedString((actual - before) / count) << " after register node\n";
	before = actual;

	sceneManager.registerNode(Util::StringIdentifier("testroot"), root.get());
	
	MinSG::destroy(root.get());
	root = NULL;
	
	actual = Util::Utils::getResidentSetMemorySize();
	std::cerr << Util::StringUtils::toFormattedString((actual - before) / count) << " after destroy\n";
	before = actual;

	return EXIT_SUCCESS;
}
