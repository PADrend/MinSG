#
# This file is part of the MinSG library.
# Copyright (C) 2014 Claudius Jähn <claudius@uni-paderborn.de>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#
minsg_add_sources(
	Material.cpp
	MaterialLib.cpp
	StreamerVOX.cpp
	VoxelWorld.cpp
)
minsg_add_extension(MINSG_EXT_VOXEL_WORLD "Defines if the MinSG extension VoxelWorld is built.")