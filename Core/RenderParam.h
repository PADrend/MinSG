/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_CORE_RENDPARAM_H
#define MINSG_CORE_RENDPARAM_H

#include <Util/StringIdentifier.h>
#include <cstdint>

namespace MinSG {

/// RenderingFlags
typedef uint32_t renderFlag_t;

enum RenderFlags {
	BOUNDING_BOXES = 1 << 0,
	NO_GEOMETRY = 1 << 1,
	FRUSTUM_CULLING = 1 << 2,
	SHOW_META_OBJECTS = 1 << 3,
	SHOW_COORD_SYSTEM = 1 << 5,
	USE_WORLD_MATRIX = 1 << 6,
	NO_STATES = 1 << 7,
	SKIP_RENDERER = 1 << 8
};

/*! Rendering parameter used during rendering.
    - flags: Rendering flags
    - channel: Name of the current renderingChannel */
class RenderParam {
	private:
		renderFlag_t flags;
		Util::StringIdentifier channel;
	public:
		RenderParam();
		RenderParam(renderFlag_t _flags);
		RenderParam(renderFlag_t _flags, const Util::StringIdentifier & _channel);

		renderFlag_t getFlags() const {
			return flags;
		}
		bool getFlag(renderFlag_t flagMask) const {
			return flags & flagMask;
		}
		void setFlag(renderFlag_t flagMask) {
			flags |= flagMask;
		}
		void setFlags(renderFlag_t _flags) {
			flags = _flags;
		}
		void unsetFlag(renderFlag_t flagMask) {
			flags -= flags & flagMask;
		}

		const RenderParam operator+(renderFlag_t flagMask) const {
			return RenderParam(flags | flagMask, channel);
		}
		const RenderParam operator-(renderFlag_t flagMask) const {
			return RenderParam(flags - (flags & flagMask), channel);
		}

		const Util::StringIdentifier & getChannel() const {
			return channel;
		}
		void setChannel(const Util::StringIdentifier & newChannel) {
			channel = newChannel;
		}
};

}

#endif /* MINSG_CORE_RENDPARAM_H */
