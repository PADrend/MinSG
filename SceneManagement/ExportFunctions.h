/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2013 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef SCENE_EXPORT_H_
#define SCENE_EXPORT_H_

#include <Util/IO/FileName.h>
#include <deque>
#include <iosfwd>

namespace MinSG {
class Node;

namespace SceneManagement {
class SceneManager;

/*!	Save MinSG nodes to a file. Throws an exception on failure.
	@param fileName Path that the new MinSG XML file will be saved to
	@param nodes Array of nodes that will be saved	*/
MINSGAPI void saveMinSGFile(SceneManager & sm, const Util::FileName & fileName, const std::deque<Node *> & nodes);

/*!	Save MinSG nodes to a stream. Throws an exception on failure.
	@param out Output stream to which the MinSG XML data will be written
	@param nodes Array of nodes that will be saved	 */
MINSGAPI void saveMinSGStream(SceneManager & sm, std::ostream & out, const std::deque<Node *> & nodes);

/*!	Traverses the scene graph below @a rootNode and saves all meshes
	that are found in GeometryNodes and that are not saved yet into PLY
	files in a separate directory.

	@param rootNode Root of scene graph that will be traversed.
	@param dirName Name of directory that is used to store the meshes.
	@param saveRegisteredNodes If true, even already saved meshes are exported.

	\todo Shouldn't this be a StdNodeVisitor or a static helper	function in Helper.cpp? */
MINSGAPI void saveMeshesInSubtreeAsPLY(Node * rootNode,const  std::string & dirName, bool saveRegisteredNodes=false);

/*!	Traverses the scene graph below @a rootNode and saves all meshes
	that are found in GeometryNodes and that are not saved yet into MMF
	files in a separate directory.

	@param rootNode Root of scene graph that will be traversed.
	@param dirName Name of directory that is used to store the meshes.
	@param saveRegisteredNodes If true, even already saved meshes are exported.
	\todo Shouldn't this be a StdNodeVisitor or a static helper	function in Helper.cpp? */
MINSGAPI void saveMeshesInSubtreeAsMMF(Node * rootNode,const  std::string & dirName, bool saveRegisteredNodes=false);
}
}

#endif // SCENE_EXPORT_H_