/*
	This file is part of the MinSG library extension SVS.
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_SVS

#ifndef MINSG_SVS_DEFINITIONS_H_
#define MINSG_SVS_DEFINITIONS_H_

namespace MinSG {
//! @ingroup ext
namespace SVS {

//! Type of interpolation to create values for a query that lies between sample points.
enum interpolation_type_t {
	//! Only use the nearest sample point.
	INTERPOLATION_NEAREST,
	//! Take the maximum of the three nearest sample points.
	INTERPOLATION_MAX3,
	//! Take the maximum of all sample points.
	INTERPOLATION_MAXALL,
	//! Weight the nearest three sample points by their distance to the query.
	INTERPOLATION_WEIGHTED3
};

}
}

#endif /* MINSG_SVS_DEFINITIONS_H_ */

#endif /* MINSG_EXT_SVS */
