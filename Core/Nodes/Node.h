/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_NODE_H
#define MINSG_NODE_H

#include "../States/State.h"
#include "../NodeVisitor.h"

#include <Geometry/Angle.h>
#include <Geometry/Box.h>
#include <Geometry/SRT.h>
#include <Geometry/Matrix4x4.h>

#include <Util/ReferenceCounter.h>
#include <Util/TypeNameMacro.h>
#include <Util/AttributeProvider.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace Geometry {
template<typename value_t> class _Box;
typedef _Box<float> Box;
}
namespace MinSG {

class FrameContext;
class GroupNode;
class RenderParam;
typedef uint32_t renderFlag_t;			//! \see RenderParam.h
typedef uint8_t renderingLayerMask_t;	//! \see RenderingLayer.h

/**
 * Base class of all nodes of the scene-graph.
 *
 *   [Node]
 *
 * \note (---|> AttributeProvider) A node's attribute's key influences its handling. \see NodeAttributeModifier.h for details.
 */
class Node :
			public Util::AttributeProvider,
			public Util::ReferenceCounter<Node> {
		PROVIDES_TYPE_NAME(Node)

	// -----------------
	public:

	/**
	 * @name Main
	 */
	//@{
	protected:
		Node();
		explicit Node(const Node & s);

	public:
		virtual ~Node();
		Node * clone(bool cloneAttributes=true)const;
		
		//! Make sure to keep a Reference to the node or call MinSG::destroy(node)
		void destroy();

	private:
		/*! ---o
			\note do not call directly. Use MinSG::destroy(node) instead. */
		virtual void doDestroy();

		/*! Prevent usage of assignment operator. */
		Node & operator=(const Node & /*n*/);

		virtual Node * doClone()const = 0;

	//@}

	// -----------------

	/**
	 * @name Parent node
	 */
	//@{
	private:
		Util::Reference<GroupNode> parentNode;

	public:
		GroupNode * getParent() const						{	return parentNode.get();	}
		Node * getRootNode();
		bool hasParent() const								{	return parentNode.isNotNull();	}

		void addToParent(Util::WeakPointer<GroupNode> p);
		Util::Reference<Node> removeFromParent();
		
		/*! Set the node's parent without removing it from its old one or informing the new parent.
			\note Use only if you really know what you are doing!!! Normally, you should use node.addToParent( newParent ); */
		void _setParent(Util::WeakPointer<GroupNode> p);
	//@}

	// -----------------

	/**
	 * @name Node flags
	 */
	//@{
	private:
		typedef uint8_t nodeFlag_t;
		nodeFlag_t nodeFlags;

		static const nodeFlag_t N_FLAG_ACTIVE = 1 << 0;
		
		/// the node (and its subtree) represent an individually rendered object
		static const nodeFlag_t N_FLAG_CLOSED_GROUP = 1 << 1;
		
		/// the node is not saved
		static const nodeFlag_t N_FLAG_TEMP_NODE = 1 << 2;

		void setFlag(nodeFlag_t f,bool value)			{	nodeFlags = value ? (nodeFlags | f) : (nodeFlags & ~f);	}
		bool getFlag(nodeFlag_t f)const 				{	return nodeFlags&f;	}

	public:
		bool isClosed()const							{	return getFlag(N_FLAG_CLOSED_GROUP);	}
		void setClosed(bool _closed=true)				{	setFlag(N_FLAG_CLOSED_GROUP,_closed);	}
		bool isActive() const							{	return getFlag(N_FLAG_ACTIVE);	}
		void activate()									{	setFlag(N_FLAG_ACTIVE, true);	}
		void deactivate()								{	setFlag(N_FLAG_ACTIVE, false);	}
		bool isTempNode()const							{	return getFlag(N_FLAG_TEMP_NODE);	}
		void setTempNode(bool b)						{	setFlag(N_FLAG_TEMP_NODE,b);	}
	//@}

	// -----------------

	/**
	 * @name Rendering layers
	 */
	//@{
	private:
		renderingLayerMask_t renderingLayers;

	public:
		renderingLayerMask_t getRenderingLayers()const					{	return renderingLayers;	}
		void setRenderingLayers(renderingLayerMask_t l)					{	renderingLayers = l;	}
		bool testRenderingLayer(renderingLayerMask_t l)const			{	return (renderingLayers&l)>0; }
	//@}

	// -----------------

	/**
	 * @name Status flags
	 */
	//@{
	private:
		typedef uint16_t statusFlag_t;
		mutable statusFlag_t statusFlags;

		static const statusFlag_t STATUS_WORLD_BB_VALID 					= 1 << 0;
		static const statusFlag_t STATUS_WORLD_MATRIX_VALID 				= 1 << 1;

		static const statusFlag_t STATUS_HAS_SRT 							= 1 << 2;
		static const statusFlag_t STATUS_MATRIX_REFLECTS_SRT 				= 1 << 3;
		static const statusFlag_t STATUS_IS_DESTROYED 						= 1 << 4;
		
		/// (internal) used to mark if the Node has a prototype
		static const statusFlag_t STATUS_IS_INSTANCE 						= 1 << 5;

		/// (internal) if the node is transformed, transformation observers (registered at a node from here to the root) are notified.
		static const statusFlag_t STATUS_TRANSFORMATION_OBSERVED			= 1 << 6;
		/// (internal) used to mark if the Node contains a transformation observer
		static const statusFlag_t STATUS_CONTAINS_TRANSFORMATION_OBSERVER	= 1 << 7;

		/// (internal) there exists a nodeAdded or nodeRemoved observer in the tree up to the root node
		static const statusFlag_t STATUS_TREE_OBSERVED						= 1 << 8;
		/// (internal) the Node contains a nodeAdded observer
		static const statusFlag_t STATUS_CONTAINS_NODE_ADDED_OBSERVER		= 1 << 9;
		/// (internal) the Node contains a nodeRemoved observer
		static const statusFlag_t STATUS_CONTAINS_NODE_REMOVED_OBSERVER		= 1 << 10;

		/// (internal) the Node contains a fixed bounding box
		static const statusFlag_t STATUS_CONTAINS_FIXED_BB					= 1 << 11;


		//! This function is const because it has to called from other const member functions. The member @a statusFlags is mutable.
		void setStatus(statusFlag_t f, bool value) const	{	statusFlags = value ? (statusFlags | f) : (statusFlags & ~f);	}
		bool getStatus(statusFlag_t statusFlag) const		{	return statusFlags & statusFlag;	}
		
		//! (internal) Automatically adjust N_STATUS_TRANSFORMATION_OBSERVED. (Further flags can be added here.)
		void updateObservedStatus();
		
	public:
		bool isDestroyed()const{	return getStatus(STATUS_IS_DESTROYED);	}
	//@}

	// -----------------

	/**
	 * @name Bounding Boxes
	 */
	//@{
	private:
		/// ---o
		virtual const Geometry::Box& doGetBB() const;
	protected:
		/*!	Call whenever the (world) bounding box may have changed. E.g. when the node is moved, a child is added or removed or a mesh is added or changed.
			The method invalidates the worldBB and traverses the tree up invalidating all worldBBs and compoundBBs (until a node has a fixed bb).	*/
		void worldBBChanged();
	public:
		//! \note the bounding box is in local coordinates
		const Geometry::Box& getBB() const;
		
		void setFixedBB(const Geometry::Box &fixedBB);
		bool hasFixedBB()const							{	return getStatus(STATUS_CONTAINS_FIXED_BB);	}
		virtual void removeFixedBB();
		/*! \note Don't store the resulting reference, but make a copy for storing: The box may change 
		when the Node is transformed. It may even get deleted when the Node's transformation is reset.	*/
		const Geometry::Box& getWorldBB() const;
	//@}

	// -----------------

	/**
	 * @name Display
	 */
	//@{
	public:
		/*! Render the node.
			\note calls doDisplay(....) internally */
		void display(FrameContext & context, const RenderParam & rp);

	protected:
		/*!  ---o
			Render the node. All matrix operations and states must be applied when called.
			This function is internally called by the default to display(...) method. */
		virtual void doDisplay(FrameContext & context, const RenderParam & rp);
	//@}

	// -----------------

	//! @name Information
	//@{
	public:
		/**
		 * Get the amount of memory that is required to store this node.
		 * 
		 * @return Amount of memory in bytes
		 */
		virtual size_t getMemoryUsage() const;
	//@}
	
	// -----------------

	/**
	 * @name Instances
	 */
	//@{
	public:
		/*!	Create a clone of the given subtree. Every new node is marked as instance of the corresponding node in the source tree.
			- Attributes are not cloned (except "(forceClone)_"-attributes), but can be accessed using findAttribute(...)
			- If a node in the source tree is already an instance, the node is cloned
			  (i.e. the new node points to the same prototype as the given node; no instances of instances).
				*/
		static Node * createInstance(const Node * source);
	
		//!	If the node is an instance, its prototype is returned; nullptr otherwise.
		Node * getPrototype()const;
		
		/*! Returns the node's attribute with the given name, if it is available; otherwise,
			if the node's prototype is available and has the attribute, it is taken from there.*/
		Util::GenericAttribute * findAttribute(const Util::StringIdentifier & key)const;
		
		//! True iff the node is an instance of another node. \see getPrototype()
		bool isInstance()const							{	return getStatus(STATUS_IS_INSTANCE);	}

		/*! (internal) Should only be called if you really know what you are doing!
			if prototype is nullptr, the STATUS_IS_INSTANCE flag is removed (otherwise set).	*/
		void _setPrototype(Node * n);
	//@}

	// -----------------

	/**
	 * @name States
	 */
	//@{
	public:
		//! Attached states and their current status.
		typedef std::vector<std::pair<Util::Reference<State>, bool>> stateList_t;

		bool hasStates()const							{	return states && (!states->empty()); }
		void addState(State * sn);
		void removeState(State * sn);
		void removeStates();
		const stateList_t * getStateListPtr()const		{	return states.get();	}
		std::vector<State*> getStates()const;

	private:
		std::unique_ptr<stateList_t> states;
	//@}

	// -----------------

	/**
	 * @name Transformation
	*	\note (internal) If the node is not transformed inside the world, 
     *		worldLocation is nullptr, the world bb equals the normal bb and the worldMatrix equals the identity.
	 */
	//@{
	private:
		struct RelativeTransformation{
			Geometry::SRT srt;
			Geometry::Matrix4x4 matrix;
		};
		mutable std::unique_ptr<RelativeTransformation> relTransformation;

		struct WorldLocation{
			Geometry::Matrix4x4 matrix_localToWorld;
			Geometry::Box worldBB;
		};
		mutable std::unique_ptr<WorldLocation> worldLocation;


		void invalidateWorldMatrix() const;
	
	protected:
		Geometry::SRT & accessSRT() const;

	public:
		float getRelScaling() const									{	return accessSRT().getScale();	}
		Geometry::Vec3 getRelOrigin() const							{	return accessSRT().getTranslation();	}
		const Geometry::Matrix4x4& getRelTransformationMatrix() const;
		const Geometry::Matrix4x4* getRelTransformationMatrixPtr() const;
		const Geometry::SRT& getRelTransformationSRT()const;
		Geometry::Vec3 getWorldOrigin() const						{	return getWorldTransformationMatrixPtr() == nullptr ? Geometry::Vec3(0,0,0) : (*getWorldTransformationMatrixPtr()).transformPosition(0,0,0);	}

		/*! Transforms from local to world coordinate system.
			\note (internal) the STATUS_WORLD_MATRIX_VALID is set to true by this function */
		const Geometry::Matrix4x4& getWorldTransformationMatrix() const;
		/*! (internal) Return the pointer to the current world matrix. If the node is transformed globally and
			no world matrix has yet been created, a new one is calculated.
			If the node is not transformed, nullptr may be returned.
			\note (internal) the STATUS_WORLD_MATRIX_VALID is set to true by this function */
		const Geometry::Matrix4x4* getWorldTransformationMatrixPtr() const;
		Geometry::SRT getWorldTransformationSRT() const; 
		
		//! Returns the inverse of getWorldTransformationMatrix()
		Geometry::Matrix4x4 getWorldToLocalMatrix() const;
		
		bool hasRelTransformation()const							{	return relTransformation!=nullptr; 	}
		bool hasRelTransformationSRT() const						{	return relTransformation!=nullptr && getStatus(STATUS_HAS_SRT);	}
		void moveRel(const Geometry::Vec3 & v)						{	accessSRT().translate(v);		transformationChanged();	}
		void moveLocal(const Geometry::Vec3 & v)					{	accessSRT().translateLocal(v);	transformationChanged();	}
		
		//! Remove all relative transformations
		void resetRelTransformation();
		void resetRelRotation()										{	accessSRT().resetRotation();	transformationChanged();	}			
		
		//!	Rotate around a local direction around the object's local origin (0,0,0).
		void rotateLocal(const Geometry::Angle & angle, const Geometry::Vec3 & v)	{	accessSRT().rotateLocal_rad(angle.rad(),v);	transformationChanged();	}

		//!	Rotate around a direction in the parent's coordinate system around the object's local origin (0,0,0).
		void rotateRel(const Geometry::Angle & angle, const Geometry::Vec3 & v)		{	accessSRT().rotateRel_rad(angle.rad(),v);	transformationChanged();}
		void scale(float f)											{	accessSRT().scale(f);	transformationChanged();	}
		void setRelScaling(float f)									{	accessSRT().setScale(f);	transformationChanged();	}
		void setRelOrigin(const Geometry::Vec3 & v)					{	accessSRT().setTranslation(v);	transformationChanged();	}
		void setRelRotation(const Geometry::Angle & angle,const Geometry::Vec3 & v){
			accessSRT().resetRotation();	
			accessSRT().rotateLocal_rad(angle.rad(),v);	
			transformationChanged();
		}
		void setRelRotation(const Geometry::Vec3 & dir,const Geometry::Vec3 & up)	{	accessSRT().setRotation(dir,up);	transformationChanged();	}

		void setRelTransformation(const Geometry::Matrix4x4 & m);
		void setRelTransformation(const Geometry::SRT & srt);
		void setWorldOrigin(const Geometry::Vec3 & v);
		void setWorldTransformation(const Geometry::SRT & worldSRT);
// ---

		const Geometry::Matrix4x4& getMatrix() const				__attribute__((deprecated)){	return getRelTransformationMatrix(); 	}
		Geometry::Vec3 getRelPosition() const 						__attribute__((deprecated)){	return getRelOrigin();	} 
		float getScale() const										__attribute__((deprecated)){	return getRelScaling();	}
		const Geometry::SRT& getSRT()const							__attribute__((deprecated)){	return getRelTransformationSRT(); 	}
		const Geometry::Matrix4x4 & getWorldMatrix() const			__attribute__((deprecated)){	return getWorldTransformationMatrix();	}
		Geometry::Vec3 getWorldPosition() const 					__attribute__((deprecated)){	return getWorldOrigin();	}
		bool hasMatrix()const 										__attribute__((deprecated)){	return hasRelTransformation(); 	}
		bool hasSRT() const											__attribute__((deprecated)){	return hasRelTransformationSRT();	}
		void reset()												__attribute__((deprecated)){	resetRelTransformation();	}
		void rotateLocal_deg(float deg, const Geometry::Vec3 & v)	__attribute__((deprecated)){	rotateLocal(Geometry::Angle::deg(deg),v);    }
		void rotateLocal_rad(float rad, const Geometry::Vec3 & v)	__attribute__((deprecated)){	rotateLocal(Geometry::Angle::rad(rad),v);    }
		void rotateRel_deg(float deg, const Geometry::Vec3 & v)		__attribute__((deprecated)){	rotateRel(Geometry::Angle::deg(deg),v);    }
		void rotateRel_rad(float rad, const Geometry::Vec3 & v)		__attribute__((deprecated)){	rotateRel(Geometry::Angle::rad(rad),v);    }
		void setScale(float f)										__attribute__((deprecated)){	setRelScaling(f);	}
		void setMatrix(const Geometry::Matrix4x4 & m)	 			__attribute__((deprecated)){	setRelTransformation( m ); 	}
		void setSRT(const Geometry::SRT & newSRT)					__attribute__((deprecated)){	return setRelTransformation(newSRT);}
		void setRelPosition(const Geometry::Vec3 & v)				__attribute__((deprecated)){	setRelOrigin(v);	}
		void setWorldPosition(const Geometry::Vec3 & v )			__attribute__((deprecated)){	setWorldOrigin(v);	}
		void setRelRotation_deg(float deg,const Geometry::Vec3 & v)	__attribute__((deprecated)){	setRelRotation(Geometry::Angle::deg(deg),v); }
		void setRelRotation_rad(float rad,const Geometry::Vec3 & v) __attribute__((deprecated)){	setRelRotation(Geometry::Angle::rad(rad),v); }
	//@}

	// -----------------

	/**
	 * @name Observation
	 */
	//@{
	protected:
		/**
		 * Inform the parent node and potentially this node that the transformation of this node has changed.
		 * This has to be called by all member functions that influence the transformation of this node.
		 * If \code isTransformationObserved() == true \endcode, all registered transformation observers (at 
		 * this node or in the nodes up to the root) are notified.
		 */
		void transformationChanged();
		//! (internal) Called by GroupNode::addNode(...)
		void informNodeAddedObservers(Node * addedNode); 
		//! (internal) Called by GroupNode::removeNode(...)
		void informNodeRemovedObservers(GroupNode * parent, Node * removedNode);
	public:
		bool isTransformationObserved() const			{	return getStatus(STATUS_TRANSFORMATION_OBSERVED);	}
		
		typedef std::function<void (Node *)> transformationObserverFunc;
		typedef std::function<void (Node *)> nodeAddedObserverFunc;
		typedef std::function<void (GroupNode *,Node *)> nodeRemovedObserverFunc;

		//! Register a function that is called whenever an observed node in the subtree is transformed.
		void addTransformationObserver(const transformationObserverFunc & func);
		//! Register a function that is called whenever a node is added somewhere in the subtree.
		void addNodeAddedObserver(const nodeAddedObserverFunc & func);
		//! Register a function that is called whenever a node is removed somewhere in the subtree.
		void addNodeRemovedObserver(const nodeRemovedObserverFunc & func);

		//! Remove all transformation observer functions.
		void clearTransformationObservers();
		//! Remove all nodeAdded observer functions.
		void clearNodeAddedObservers();
		//! Remove all nodeAdded observer functions.
		void clearNodeRemovedObservers();
	//@}

	// -----------------

	/**
	 * @name Traversal
	 */
	//@{
	public:
		///  ---o
		virtual NodeVisitor::status traverse(NodeVisitor & visitor);
	//@}

	// -----------------

};

}

#endif // MINSG_NODE_H
