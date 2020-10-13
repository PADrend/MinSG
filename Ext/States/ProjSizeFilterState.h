/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef PROJ_SIZE_FILTER_H_
#define PROJ_SIZE_FILTER_H_

#include "../../Core/States/NodeRendererState.h"

namespace MinSG {
class FrameContext;
enum class NodeRendererResult : bool;

/*!	Nodes with a small projected size, further than minimum distance to the observer are
	moved to another RenderingChannel.
	\note Should not be used together with a renderer which takes control over the rendering process.
	@ingroup states
*/
class ProjSizeFilterState : public NodeRendererState {
		PROVIDES_TYPE_NAME(ProjSizeFilterState)

		float maximumProjSize;
		float minimumDistance;

		Util::StringIdentifier targetChannel;

		bool forceClosed; //! all leaf nodes are moved, independent from their projected size.

		MINSGAPI NodeRendererResult displayNode(FrameContext & context, Node * node, const RenderParam & rp) override;
	public:
		MINSGAPI ProjSizeFilterState();

		Util::StringIdentifier getTargetChannel() const	{	return targetChannel;	}
		void setTargetChannel(Util::StringIdentifier s) 	{	targetChannel = s;		}

		float getMinimumDistance() const			{	return minimumDistance;	}
		void setMinimumDistance(float d) 			{	minimumDistance = d;	}

		float getMaximumProjSize() const			{	return maximumProjSize;	}
		void setMaximumProjSize(float s) 			{	maximumProjSize = s;	}

		bool isForceClosed() const					{	return forceClosed;		}
		void setForceClosed(bool b) 				{	forceClosed = b;		}

		MINSGAPI ProjSizeFilterState * clone() const override;
};

}

#endif // PROJ_SIZE_FILTER_H_
