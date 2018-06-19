#
# This file is part of the MinSG library.
# Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#
minsg_add_sources(
	ABTreeBuilder.cpp
	ABTree.cpp
	Conversion.cpp
	kDTreeBuilder.cpp
	kDTree.cpp
	OctreeBuilder.cpp
	Octree.cpp
	RandomizedSampleTreeBuilder.cpp
	RandomizedSampleTree.cpp
	TreeVisualization.cpp
	TriangleAccessor.cpp
	TriangleTreeBuilder.cpp
	TriangleTree.cpp
)

minsg_add_extension(MINSG_EXT_TRIANGLETREES "Defines if the MinSG extension for trees (e.g. k-D-tree, octree), which work on triangles, is built." ON)
