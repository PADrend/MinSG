/*
	This file is part of the MinSG library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "GraphVizOutput.h"
#include "StdNodeVisitors.h"
#include "../Core/Nodes/Node.h"
#include "../SceneManagement/SceneManager.h"
#include <Util/IO/FileName.h>
#include <Util/IO/FileUtils.h>
#include <cstdint>
#include <sstream>

namespace MinSG {
namespace GraphVizOutput {

/**
 * Write the GraphViz output recursively for the given subtree.
 *
 * @param node Root of a MinSG subgraph which shall be output.
 * @param sceneManager Optional scene manager to resolve registered node, or
 * @c nullptr to deactivate name output.
 * @param output Output stream that will be used for writing in DOT format.
 */
static void writeGraphVizOutput(Node * node,
								const SceneManagement::SceneManager * sceneManager,
								std::ostream & output) {
	std::string typeName(node->getTypeName());
	std::string style;
	if(typeName == "GeometryNode") {
		typeName = "G";
		style = ",shape=octagon";
	} else if(typeName == "ListNode") {
		typeName = "L";
		style = ",shape=rect";
	}
	std::string extraLabel;
	if(sceneManager != nullptr) {
		const auto id = sceneManager->getNodeId(node);
		if(!id.empty()) {
			extraLabel = "\\n\\\"";
			extraLabel += id.toString();
			extraLabel += "\\\"";
		}
	}
	output << '\t' << reinterpret_cast<uintptr_t>(node) << "[label=\"" << typeName << extraLabel << '\"' << style << "];\n";
	const auto children = getChildNodes(node);
	for(const auto & child : children) {
		output << '\t'<< reinterpret_cast<uintptr_t>(node) << "->" << reinterpret_cast<uintptr_t>(child) << ";\n";
		writeGraphVizOutput(child, sceneManager, output);
	}
}

void treeToFile(Node * rootNode, 
				const SceneManagement::SceneManager * sceneManager,
				const Util::FileName & fileName) {
	std::ostringstream output;
	output << "digraph g {\n";
	output << "\tnode [width=0.01,height=0.01];\n";
	writeGraphVizOutput(rootNode, sceneManager, output);
	output << "}\n";
	const auto str = output.str();
	Util::FileUtils::saveFile(fileName, std::vector<uint8_t>(str.cbegin(), str.cend()));
}

}
}
