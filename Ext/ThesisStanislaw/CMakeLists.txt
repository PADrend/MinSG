#
# This file is part of the MinSG library.
# Copyright (C) 2016 Stanislaw Eppinger
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#

minsg_add_sources(
	ApproxSceneDebug.cpp
	LightPatchRenderer.cpp
	PhongGI.cpp
	PhotonRenderer.cpp
	PhotonSampler.cpp
	PolygonIndexing.cpp
)

minsg_add_extension(MINSG_EXT_THESISSTANISLAW "Defines if the MinSG extension for Stanislaws' Master's Thesis is built.")
