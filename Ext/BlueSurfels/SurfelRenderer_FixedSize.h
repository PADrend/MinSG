/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2014 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2016-2017 Sascha Brandt <myeti@mail.uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#ifndef SURFEL_RENDERER_FIXE_SIZE_H_
#define SURFEL_RENDERER_FIXE_SIZE_H_

#include "../../Core/States/NodeRendererState.h"
#include "../../Core/Nodes/CameraNode.h"

#include <Util/Timer.h>

#include <Geometry/Vec3.h>

#include <deque>
#include <set>

namespace Rendering {
class Mesh;
} 

namespace MinSG{

namespace BlueSurfels {
	
class SurfelRendererFixedSize : public NodeRendererState {
	PROVIDES_TYPE_NAME(SurfelRendererFixedSize)
	public:
		SurfelRendererFixedSize();
		SurfelRendererFixedSize(const SurfelRendererFixedSize&) = default;
		virtual ~SurfelRendererFixedSize();
		
		NodeRendererResult displayNode(FrameContext & context, Node * node, const RenderParam & rp) override;

		float getCountFactor()const		{	return countFactor;	}
		void setCountFactor(float f)	{	countFactor = f;	}
				
		float getSizeFactor()const		{	return sizeFactor;	}
		void setSizeFactor(float f)		{	sizeFactor = f;	}
				
		float getSurfelSize()const		{	return surfelSize;	}
		void setSurfelSize(float f)		{	surfelSize = f;	}
		
		float getMaxSurfelSize()const		{	return maxSurfelSize;	}
		void setMaxSurfelSize(float f)		{	maxSurfelSize = f;	}
		
		bool getDebugHideSurfels() const { return debugHideSurfels; }
		void setDebugHideSurfels(bool b) { debugHideSurfels = b; }
		
		bool isDebugCameraEnabled() const { return debugCameraEnabled; }
		void setDebugCameraEnabled(bool b) { debugCamera = nullptr; debugCameraEnabled = b; };
		
		bool getDeferredSurfels() const { return deferredSurfels; }
		void setDeferredSurfels(bool b) { deferredSurfels = b; }
		
		bool isAdaptive() const	{	return adaptive;	}
		void setAdaptive(bool b)	{	adaptive = b;	}
		
		bool isFoveated() const	{	return foveated;	}
		void setFoveated(bool b)	{	foveated = b;	}
		
		bool getDebugFoveated() const	{	return debugFoveated;	}
		void setDebugFoveated(bool b)	{	debugFoveated = b;	}
		
		const std::vector<std::pair<float, float>>& getFoveatZones()const {	return foveatZones;	}
		void setFoveatZones(const std::vector<std::pair<float, float>>& zones) {	
			foveatZones.clear();
			foveatZones.assign(zones.begin(), zones.end()); 
			std::sort(foveatZones.begin(), foveatZones.end());	
		}
		
		float getMaxFrameTime() const		{	return maxFrameTime;	}
		void setMaxFrameTime(float f)		{	maxFrameTime = f;	}
		
		SurfelRendererFixedSize* clone() const override	{	return new SurfelRendererFixedSize(*this);	}
		
		void drawSurfels(FrameContext & context, float minSize=0, float maxSize=1024) const;
	protected:
		stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;
		void doDisableState(FrameContext & context, Node * node, const RenderParam & rp) override;
		
		// (mesh, surfel count, median distance)
		typedef std::tuple<Rendering::Mesh*, uint32_t, float> Surfels_t; 
		virtual Surfels_t getSurfelsForNode(FrameContext & context, Node * node);
				
		float getMedianDist(Node * node, Rendering::Mesh& mesh);
	private:			
		float countFactor,sizeFactor,surfelSize,maxSurfelSize,maxFrameTime;
		bool debugHideSurfels, debugCameraEnabled, deferredSurfels, adaptive, foveated, debugFoveated = false;
		Util::Reference<CameraNode> debugCamera;
		// distance to camera, node, prefix length, size
		typedef std::tuple<float,Node*,uint32_t,float> SurfelAssignment_t;
		std::set<SurfelAssignment_t> deferredSurfelQueue;
		Util::Timer frameTimer;
		std::vector<std::pair<float, float>> foveatZones;
};
}

}

#endif // SURFEL_RENDERER_FIXE_SIZE_H_
#endif // MINSG_EXT_BLUE_SURFELS
