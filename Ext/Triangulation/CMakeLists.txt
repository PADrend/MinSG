#
# This file is part of the MinSG library.
# Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#
minsg_add_sources(
	Delaunay2d.cpp
	Delaunay3d.cpp
	Helper.cpp
)

minsg_add_extension(MINSG_EXT_TRIANGULATION "Defines if the MinSG extension Triangulation is built (based on external library detri).")
minsg_add_dependencies(MINSG_EXT_TRIANGULATION MINSG_HAVE_LIB_DETRI)
