/*
	This file is part of the MinSG library.
	Copyright (C) 2012-2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include <MinSG/Core/Nodes/GeometryNode.h>
#include <MinSG/Ext/ValuatedRegion/ValuatedRegionNode.h>
#include <MinSG/Ext/VisibilitySubdivision/VisibilityVector.h>
#include <MinSG/Helper/Helper.h>
#include <MinSG/Helper/StdNodeVisitors.h>
#include <MinSG/SceneManagement/SceneManager.h>
#include <Geometry/Box.h>
#include <Geometry/Vec3.h>
#include <Util/IO/FileName.h>
#include <Util/IO/TemporaryDirectory.h>
#include <Util/GenericAttribute.h>
#include <Util/Macros.h>
#include <Util/StringUtils.h>
#include <Util/Timer.h>
#include <bitset>
#include <cstdlib>
#include <iostream>
#include <sstream>

int test_valuated_region_node() {
#ifdef MINSG_EXT_VISIBILITY_SUBDIVISION
	std::cout << "Test export and import of ValuatedRegionNodes ... ";
	
	Util::TemporaryDirectory tempDir("ValuatedRegionNodeTest");
	Util::FileName tempFile = tempDir.getPath();
	tempFile.setFile("test_valuated_region_node.minsg");
	
	Util::Timer timer;
	timer.reset();
	
	using namespace MinSG::VisibilitySubdivision;
	MinSG::SceneManagement::SceneManager sceneManager;
	
	const uint32_t count = 10;
		
	Util::Reference<MinSG::GeometryNode> geoNodes[count];
	for(uint_fast32_t i = 0; i < count; ++i) {
		geoNodes[i] = new MinSG::GeometryNode;
		sceneManager.registerNode(std::string("Node") + Util::StringUtils::toString(i), geoNodes[i].get());
	}
	
	{ // ----- EXPORT -----
		Util::Reference<MinSG::ValuatedRegionNode> rootRegion = new MinSG::ValuatedRegionNode(Geometry::Box(-1, 1, -1, 1, -1, 1), Geometry::Vec3i((1 << count), 1, 1));
		for(uint_fast32_t i = 0; i < (1 << count); ++i) {
			const std::bitset<count> visibleSet(i);
			VisibilityVector vv;
			for(uint_fast32_t n = 0; n < count; ++n) {
				if(visibleSet[n]) {
					vv.setNode(geoNodes[n].get(), n);
				}
			}
			
			MinSG::ValuatedRegionNode * valuatedRegion = new MinSG::ValuatedRegionNode(Geometry::Box(-1, 1, -1, 1, -1, 1), Geometry::Vec3i(1, 1, 1));
			Util::GenericAttributeList * genericList = new Util::GenericAttributeList;
			genericList->push_back(new VisibilityVectorAttribute(vv));
			valuatedRegion->setValue(genericList);
			
			rootRegion->addChild(valuatedRegion);
		}
		std::deque<MinSG::Node *> nodes;
		nodes.push_back(rootRegion.get());
		sceneManager.saveMinSGFile(tempFile, nodes);
		MinSG::destroy(rootRegion.get());
	}
	
	{ // ----- IMPORT -----
		std::deque<Util::Reference<MinSG::Node>> nodes = sceneManager.loadMinSGFile(tempFile);
		
		if(nodes.size() != 1) {
			std::cout << "Import failed (line " << __LINE__ << "): Not one but " << nodes.size() << " nodes loaded." << std::endl;
			return EXIT_FAILURE;
		}
		
		Util::Reference<MinSG::ValuatedRegionNode> rootRegion = dynamic_cast<MinSG::ValuatedRegionNode *>(nodes.front().get());
		if(rootRegion == NULL) {
			std::cout << "Import failed (line " << __LINE__ << "): Node has wrong type." << std::endl;
			return EXIT_FAILURE;
		}
		
		const auto children = MinSG::getChildNodes(rootRegion.get());
		
		if(children.size() != (1 << count)) {
			std::cout << "Import failed (line " << __LINE__ << "): Not " << (1 << count) << " but " << children.size() << " child nodes loaded." << std::endl;
			return EXIT_FAILURE;
		}
		
		for(uint_fast32_t i = 0; i < (1 << count); ++i) {
			MinSG::ValuatedRegionNode * valuatedRegion = dynamic_cast<MinSG::ValuatedRegionNode *>(children[i]);
			if(valuatedRegion == NULL) {
				std::cout << "Import failed (line " << __LINE__ << "): Child node has wrong type." << std::endl;
				return EXIT_FAILURE;
			}
			Util::GenericAttributeList * genericList = dynamic_cast<Util::GenericAttributeList *>(valuatedRegion->getValue());
			if(genericList == NULL) {
				std::cout << "Import failed (line " << __LINE__ << "): Value is no GenericAttributeList." << std::endl;
				return EXIT_FAILURE;
			}
			if(genericList->size() != 1) {
				std::cout << "Import failed (line " << __LINE__ << "): GenericAttributeList has not one but " << genericList->size() << " entries." << std::endl;
				return EXIT_FAILURE;
			}
			VisibilityVectorAttribute * vva = dynamic_cast<VisibilityVectorAttribute *>(genericList->front());
			if(vva == NULL) {
				std::cout << "Import failed (line " << __LINE__ << "): GenericAttributeList does not contain a VisibilityVector." << std::endl;
				return EXIT_FAILURE;
			}
			VisibilityVector & vv = vva->ref();
			
			const std::bitset<count> visibleSet(i);
			for(uint_fast32_t n = 0; n < count; ++n) {
				if(visibleSet[n]) {
					if(vv.getBenefits(geoNodes[n].get()) != n) {
						std::cout << "Import failed (line " << __LINE__ << "): VisibilityVector contains wrong values." << std::endl;
						return EXIT_FAILURE;
					}
				}
			}
		}
		
		MinSG::destroy(rootRegion.get());
	}
	
	timer.stop();
	std::cout << "done (duration: " << timer.getSeconds() << " s).\n";
#endif /* MINSG_EXT_VISIBILITY_SUBDIVISION */
	return EXIT_SUCCESS;
}
