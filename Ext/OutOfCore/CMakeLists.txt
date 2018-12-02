#
# This file is part of the MinSG library.
# Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#
minsg_add_sources(
	CacheContext.cpp
	CacheLevel.cpp
	CacheLevelFiles.cpp
	CacheLevelFileSystem.cpp
	CacheLevelGraphicsMemory.cpp
	CacheLevelMainMemory.cpp
	CacheManager.cpp
	CacheObject.cpp
	DataStrategy.cpp
	ImportHandler.cpp
	MeshAttributeSerialization.cpp
	OutOfCore.cpp
)

minsg_add_extension(MINSG_EXT_OUTOFCORE "Defines if the MinSG extension for external memory algorithms is built." ${MINSG_RECOMMENDED_EXT})
