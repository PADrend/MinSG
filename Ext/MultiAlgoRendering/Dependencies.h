/*
	This file is part of the MinSG library extension MultiAlgoRendering.
	Copyright (C) 2013 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_MULTIALGORENDERING

#ifndef MAR_DEPENDENCIES_H
#define MAR_DEPENDENCIES_H

#ifndef MINSG_EXT_SPHERICALSAMPLING
#error "MINSG_EXT_MULTIALGORENDERING requires MINSG_EXT_SPHERICALSAMPLING"
#endif

#ifndef MINSG_EXT_BLUE_SURFELS
#error "MINSG_EXT_MULTIALGORENDERING requires MINSG_EXT_BLUE_SURFELS"
#endif

#ifndef MINSG_EXT_COLORCUBES
#error "MINSG_EXT_MULTIALGORENDERING requires MINSG_EXT_COLORCUBES"
#endif

#endif // MAR_DEPENDENCIES_H
#endif // MINSG_EXT_MULTIALGORENDERING
