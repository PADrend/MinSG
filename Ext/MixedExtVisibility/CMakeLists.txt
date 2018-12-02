#
# This file is part of the MinSG library.
# Copyright (C) 2015 Claudius J�hn <claudius@uni-paderborn.de>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#
minsg_add_sources(
	Preprocessor.cpp
)

minsg_add_extension(MINSG_EXT_MIXED_EXTERN_VISIBILITY "Defines if the MinSG extension MixedExternVisibility is built." ${MINSG_RECOMMENDED_EXT})
