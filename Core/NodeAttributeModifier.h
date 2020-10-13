/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2013 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2013 Ralf Petring <ralf@petring.net>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_NODE_ATTRIBUTE_MODIFIER_H
#define MINSG_NODE_ATTRIBUTE_MODIFIER_H

#include <Util/StringIdentifier.h>

namespace MinSG {

/** A Node's attributes behave differently according to an optional prefix: "$prefix$rest of the key"
 * The prefix can consist of the following modifiers:
 *   - C ... copy to [C]lone (default)
 *   - c ... don't copy to clone
 *   - I ... copy to [Instance]
 *   - i ... don't copy to instance (default)
 *   - S ... [S]ave to file (default)
 *   - s ... don't save to file
 * @ingroup helper
 */
namespace NodeAttributeModifier {

static const uint32_t COPY_TO_CLONES		= (1<<0);
static const uint32_t COPY_TO_INSTANCES		= (1<<1);
static const uint32_t SAVE_TO_FILE			= (1<<2);
static const uint32_t PRIVATE_ATTRIBUTE		= 0;
static const uint32_t DEFAULT_ATTRIBUTE		= COPY_TO_CLONES|SAVE_TO_FILE;

MINSGAPI Util::StringIdentifier create(const std::string& mainKey, uint32_t flags );

MINSGAPI uint32_t getFlags(const std::string &);
inline uint32_t getFlags(const Util::StringIdentifier & key)			{		return getFlags(key.toString());	}

inline bool isCopiedToClone( const Util::StringIdentifier & key)		{		return getFlags(key)&COPY_TO_CLONES;	}
inline bool isCopiedToInstance( const Util::StringIdentifier& key)		{		return getFlags(key)&COPY_TO_INSTANCES;	}
inline bool isSaved(const Util::StringIdentifier& key)					{		return getFlags(key)&SAVE_TO_FILE;	}
inline bool isSaved(const std::string& key)								{		return getFlags(key)&SAVE_TO_FILE;	}

}
}

#endif // MINSG_NODE_ATTRIBUTE_MODIFIER_H
