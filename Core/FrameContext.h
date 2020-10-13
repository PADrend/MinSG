/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_CORE_FRAMECONTEXT_H
#define MINSG_CORE_FRAMECONTEXT_H

#include "NodeRenderer.h"
#include "RenderParam.h"

#include <Util/ReferenceCounter.h>
#include <Util/References.h>
#include <Util/Registry.h>
#include <Util/StringIdentifier.h>
#include <Geometry/Vec4.h>
#include <functional>
#include <list>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations.
namespace Geometry {
template<typename _T> class _Rect;
typedef _Rect<float> Rect;
template<typename _T> class _Vec3;
typedef _Vec3<float> Vec3;
}
namespace Rendering {
class Mesh;
class RenderingContext;
class TextRenderer;
}
namespace Util {
class Color4ub;
class Color4f;
}

namespace MinSG {
class Node;
class AbstractCameraNode;
class Statistics;
class State;

// -----------------------------------

//!	FrameContext
class FrameContext : public Util::ReferenceCounter<FrameContext>{

	/*!	@name Main */
	//	@{
	public:
		MINSGAPI FrameContext();
		MINSGAPI ~FrameContext();
	//	@}

	// -----------------------------------

	/*!	@name Camera (and Projection) */
	// @{
	private:
		Util::Reference<AbstractCameraNode> camera;
		std::stack<Util::Reference<AbstractCameraNode>> cameraStack;
	public:
		//!	Check if the context has a camera.
		bool hasCamera() const 											{	return !camera.isNull();	}

		//!	@return The associated camera or @c nullptr, if no camera is associated.
		AbstractCameraNode * getCamera()								{	return camera.get();	}

		//!	@return The associated camera or @c nullptr, if no camera is associated.
		const AbstractCameraNode * getCamera() const					{	return camera.get();	}

		/*!	Associate a new camera with the context.
			@param newCamera New camera or @c nullptr, if the camera should be removed.*/
		MINSGAPI void setCamera(AbstractCameraNode * newCamera);

		//! Push the current camera onto the camera stack.
        MINSGAPI void pushCamera();

		//! Pop a camera from the top of the camera stack and make it the current camera.
		MINSGAPI void popCamera();

		void pushAndSetCamera(AbstractCameraNode * newCamera) {
			pushCamera();
			setCamera(newCamera);
		}

		MINSGAPI Geometry::Vec3 convertWorldPosToScreenPos(const Geometry::Vec3 & objPos) const;
		MINSGAPI Geometry::Vec3 convertScreenPosToWorldPos(const Geometry::Vec3 & screenPos) const;

		/*! Projects the BoundingBox of the given Node into the given screenRect
			using the current camera and projection matrix of the RenderingContext. */
		MINSGAPI Geometry::Rect getProjectedRect(Node * node, const Geometry::Rect & screenRect) const;

		/*! Projects the BoundingBox of the given Node into the current view port
			using the current camera and projection matrix of the RenderingContext. */
		MINSGAPI Geometry::Rect getProjectedRect(Node * node) const;
	// @}

	// -----------------------------------

	/*!	@name visual world coordinate system */
	// @{
	private:
		Geometry::Vec3 worldUpVector;
		Geometry::Vec3 worldFrontVector;
		Geometry::Vec3 worldRightVector;
	public:
		void setWorldUpVector(const Geometry::Vec3 & v) {
			worldUpVector = v;
			worldUpVector.normalize();
		}
		void setWorldFrontVector(const Geometry::Vec3 & v) {
			worldFrontVector = v;
			worldFrontVector.normalize();
		}
		void setWorldRightVector(const Geometry::Vec3 & v) {
			worldRightVector = v;
			worldRightVector.normalize();
		}
		const Geometry::Vec3 & getWorldUpVector() const {
			return worldUpVector;
		}
		const Geometry::Vec3 & getWorldFrontVector() const {
			return worldFrontVector;
		}
		const Geometry::Vec3 & getWorldRightVector() const {
			return worldRightVector;
		}
	// @}

	// -----------------------------------

	/*!	@name Frame handling */
	// @{
	private:
		int frameNumber; // <- only used for statistics
	public:
		/*!	- Initializes rendering statistics (Statistics & FrameStats).
			- Inform Rendering::MeshDataStrategy about the start of a new frame.
			- Inform the frameListeners about the start of a new frame by calling onBeginFrame().
			@param frameNumber If <0 the internal frameNumber is taken and increased; used for statistics	*/
		MINSGAPI void beginFrame(int frameNumber = -1);

		/*!	- Mark the end of the frame for the rendering statistics (Statistics & FrameStats)
			@param waitForGLfinish defines if finish on the rendering context is called before marking the end of the frame	*/
		MINSGAPI void endFrame(bool waitForGlFinish = false);


		/*!	Called before the next frame starts
			The listener may register event listeners at the given FrameContext.
			@param	FrameContext
			@return finished?
				true if FrameListener should be removed and deleted
				false if FrameListener should be executed again next frame	*/

		typedef std::function<bool (FrameContext &)> FrameListenerFunction;

		/*!	Register a new event listener.
			@param listener New event listener	*/
		MINSGAPI void addBeginFrameListener(const FrameListenerFunction & listener);
		
		/*!	Register a new event listener.
		 @ param listener New event listener	*/                             
		MINSGAPI void addEndFrameListener(const FrameListenerFunction & listener);

	private:
		std::vector<FrameListenerFunction> beginFrameListenerCallbacks;
		std::vector<FrameListenerFunction> endFrameListenerCallbacks;
	//	@}

	// -----------------------------------

	/*!	@name Rendering  */
	//	@{
	public:
		typedef Util::Registry<std::list<NodeRenderer>> renderingChannel_t;
		typedef renderingChannel_t::handle_t node_renderer_registration_t;
		typedef std::unordered_map<Util::StringIdentifier, renderingChannel_t> channelMap_t;

		MINSGAPI static const Util::StringIdentifier DEFAULT_CHANNEL;
		MINSGAPI static const Util::StringIdentifier TRANSPARENCY_CHANNEL;
		MINSGAPI static const Util::StringIdentifier APPROXIMATION_CHANNEL;

		/*! Renders the node with the current renderer of the given rendering channel (rp.channel).
			@return true if the node could be handled by a renderer. */
		MINSGAPI bool displayNode(Node * node, const RenderParam & rp);

		MINSGAPI node_renderer_registration_t registerNodeRenderer(const Util::StringIdentifier & channelName, NodeRenderer renderer);
		MINSGAPI void unregisterNodeRenderer(const Util::StringIdentifier & channelName, node_renderer_registration_t handle);

		const channelMap_t & getRenderingChannels() const {
			return renderingChannels;
		}

		Rendering::RenderingContext & getRenderingContext() 				{	return *renderingContext;	}
		const Rendering::RenderingContext & getRenderingContext() const 	{	return *renderingContext;	}

		/*! Display @p mesh. The mesh is uploaded if necessary and the number of triangles is
		 counted for the frameStats (if enabled)	*/
		MINSGAPI void displayMesh(Rendering::Mesh * mesh);
		MINSGAPI void displayMesh(Rendering::Mesh * mesh,uint32_t firstElement,uint32_t elementCount);

		MINSGAPI void showAnnotation(Node * node, const std::string & text, const int yPosOffset,const bool showRectangle, const Util::Color4f & textColor,const Util::Color4f & bgColor);
		MINSGAPI void showAnnotation(Node * node, const std::string & text, const int yPosOffset = 0,const bool showRectangle = true);
	private:
		std::unique_ptr<Rendering::RenderingContext> renderingContext;
		channelMap_t renderingChannels;
	//	@}

	// -----------------------------------
		
	//!	@name Text Rendering
	//	@{
	private:
		std::unique_ptr<Rendering::TextRenderer> textRenderer;

	public:
		/**
		 * Set the default text renderer. The text renderer is used to display
		 * text (e.g., annotations of nodes).
		 * 
		 * @param newTextRenderer A copy of the given renderer will be stored as
		 * default text renderer.
		 */
		MINSGAPI void setTextRenderer(const Rendering::TextRenderer & newTextRenderer);

		/**
		 * Access the default text renderer. The text renderer can be used to
		 * display text.
		 * 
		 * @return Default text renderer
		 */
		MINSGAPI const Rendering::TextRenderer & getTextRenderer() const;
	//	@}

	/*!	@name Statistics */
	//	@{
	private:
		std::unique_ptr<Statistics> statistics;
	public:
		Statistics & getStatistics() 			{	return *(statistics.get());	}
	//	@}

};
}

#endif // MINSG_CORE_FRAMECONTEXT_H
