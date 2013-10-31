/*
	This file is part of the MinSG library extension Profiling.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_PROFILING_ACTION_H_
#define MINSG_PROFILING_ACTION_H_

#include <Util/StringIdentifier.h>

namespace Util {
class GenericAttributeMap;
}
namespace MinSG {
namespace Profiling {

//! Forward declaration of Action
typedef Util::GenericAttributeMap Action;

static const Util::StringIdentifier ATTR_description("description");
static const Util::StringIdentifier ATTR_memoryBegin("memoryBegin");
static const Util::StringIdentifier ATTR_memoryEnd("memoryEnd");
static const Util::StringIdentifier ATTR_timeBegin("timeBegin");
static const Util::StringIdentifier ATTR_timeEnd("timeEnd");

}
}

#endif /* MINSG_PROFILING_ACTION_H_ */
