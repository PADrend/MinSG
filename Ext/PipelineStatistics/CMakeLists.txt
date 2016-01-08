#
# This file is part of the MinSG library extension Pipeline Statistics.
# Copyright (C) 2016 Benjamin Eikel <benjamin@eikel.org>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#
minsg_add_sources(
	Collector.cpp
	Statistics.cpp
)

minsg_add_extension(MINSG_EXT_PIPELINESTATISTICS "Defines if the MinSG extension Pipeline Statistics is built.")
