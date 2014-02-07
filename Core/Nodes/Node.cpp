/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "Node.h"
#include "GroupNode.h"
#include "../FrameContext.h"
#include "../NodeAttributeModifier.h"
#include "../States/State.h"
#include "../../Helper/StdNodeVisitors.h"
#include <Geometry/BoxHelper.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/DrawCompound.h>
#include <Util/Macros.h>
#include <Util/ObjectExtension.h>

namespace MinSG{

// ---- Main

//! (ctor)
Node::Node() :
		Util::AttributeProvider(), ReferenceCounter_t(),
		parentNode(),
		nodeFlags(N_FLAG_ACTIVE|N_FLAG_CLOSED_GROUP),
		statusFlags(0),
		states(){
	//ctor
}

//! (ctor)
Node::Node(const Node & source) :
		Util::AttributeProvider(), ReferenceCounter_t(),
		parentNode(),
		nodeFlags(source.nodeFlags),
		statusFlags(0),
		states() {

	// do not copy any attributes; these have to be handled by the cloning or instancing routine
	
	if (source.hasSRT()){
		setSRT(source.getSRT());
	}else if (source.hasMatrix()) {
		setMatrix(source.getMatrix());
	}

	if(source.hasStates()){
		for(const auto & stateEntry : *source.states) {
			addState(stateEntry.first.get());
		}
	}
	
	if(source.isInstance())
		_setPrototype( source.getPrototype() );
	
	if(source.hasFixedBB())
		setFixedBB( source.getBB() );
	
	//ctor
}

//! (dtor)
Node::~Node() {
	if(getStatus(STATUS_CONTAINS_TRANSFORMATION_OBSERVER))
		clearTransformationObservers();
	if(getStatus(STATUS_CONTAINS_NODE_ADDED_OBSERVER))
		clearNodeAddedObservers();
	if(getStatus(STATUS_CONTAINS_NODE_REMOVED_OBSERVER))
		clearNodeRemovedObservers();
	_setParent(nullptr);
	removeStates();
}


Node * Node::getRootNode() {
	Node * root = this;
	while(root->hasParent())
		root = root->getParent();
	return root;
}


void Node::addToParent(Util::WeakPointer<GroupNode> p){
	if(p.isNull()){
		removeFromParent();
	}else{
		p->addChild(this);
	}
}

Util::Reference<Node> Node::removeFromParent(){
	Util::Reference<Node> refHolder(this);
	if(hasParent()){
		getParent()->removeChild(this);
	}
	return refHolder;
}

void Node::_setParent(Util::WeakPointer<GroupNode> p){	
	if( p!=parentNode.get()){	
		parentNode = p;	
		invalidateWorldMatrix();
		updateObservedStatus();
	}	
}

// -------------------------------------------------------
// BB (Bounding Box)
static Util::StringIdentifier attrName_fixedBB(NodeAttributeModifier::create("fixedBB", NodeAttributeModifier::PRIVATE_ATTRIBUTE)); // don't save, copy to clones or to instances

void Node::setFixedBB(const Geometry::Box &fixedBB)	{
	if(hasFixedBB()){
		(*Util::requireObjectExtension<Geometry::Box>(attrName_fixedBB,this)) = fixedBB;
	}else{
		(*Util::addObjectExtension<Geometry::Box>(attrName_fixedBB,this)) = fixedBB;
		setStatus(STATUS_CONTAINS_FIXED_BB,true);
	}
	worldBBChanged();
}

void Node::removeFixedBB(){
	if(hasFixedBB()){
		setStatus(STATUS_CONTAINS_FIXED_BB,false);
		AttributeProvider::unsetAttribute(attrName_fixedBB);
		worldBBChanged();
	}
}
		
const Geometry::Box& Node::getBB() const {
	if(hasFixedBB()){
		return *Util::requireObjectExtension<Geometry::Box>(attrName_fixedBB,this);
	}else{
		return doGetBB();
	}
}

//! ---o
const Geometry::Box& Node::doGetBB() const {
	static const Geometry::Box bb;
	// Standard box constructor returns zero box.
	return bb;
}

void Node::worldBBChanged(){
	setStatus(STATUS_WORLD_BB_VALID, false);
	for(GroupNode * p = this->getParent();p && !p->hasFixedBB();p=p->getParent()){
		p->invalidateCompoundBB();
		p->setStatus(STATUS_WORLD_BB_VALID, false);;
	}
}

//------------------------------------------

void Node::display(FrameContext & context, const RenderParam & rp) {
	if (!isActive())
		return;

	bool skipRendering=false;
	bool matrixMustBePopped=false;

	// - apply transformations
	if( (rp.getFlag(USE_WORLD_MATRIX))>0 && getWorldMatrixPtr()!=nullptr ){
		matrixMustBePopped = true;
		context.getRenderingContext().pushMatrix();
		context.getRenderingContext().resetMatrix();
		context.getRenderingContext().multMatrix(*getWorldMatrixPtr());
	}else{
		const Geometry::Matrix4x4 * m=getMatrixPtr();
		if (m) {
			matrixMustBePopped = true;
			context.getRenderingContext().pushMatrix();
			context.getRenderingContext().multMatrix(*m);
		}
	}

	// - enable states
	if(hasStates() && !(rp.getFlag(NO_STATES))) {
		for(auto & stateEntry : *states) {
			const State::stateResult_t result = stateEntry.first->enableState(context, this, rp);
			if(result == State::STATE_OK) {
				stateEntry.second = true;
			} else if(result == State::STATE_SKIPPED) {
				stateEntry.second = false;
			} else if(result == State::STATE_SKIP_OTHER_STATES) {
				stateEntry.second = true;
				break;
			} else if(result == State::STATE_SKIP_RENDERING) {
				// if the state returned STATE_SKIP_RENDERING it should be active afterwards, so that disableState must not be called.
				stateEntry.second = false;
				skipRendering = true;
				break;
			}
		}
	}

	// - perform rendering of the node itself
	if(!skipRendering){
		if (rp.getFlag(SHOW_COORD_SYSTEM)) {
			Rendering::drawCoordSys(context.getRenderingContext(), hasParent()?1.0f:10000.0f);
		}
		doDisplay(context,rp);
	}

	// - disable states
	if (hasStates()&& !(rp.getFlag(NO_STATES))) {
		for (auto it=states->rbegin();it!=states->rend();++it) {
			// Only disable the state if it was enabled successfully before.
			if(it->second) {
				it->first->disableState(context,this,rp);
				it->second = false;
			}
		}
	}

	// - revert transformations
	if (matrixMustBePopped ){
		context.getRenderingContext().popMatrix();
	}
}

//! ---o
void Node::doDisplay(FrameContext & /*context*/, const RenderParam & /*rp*/){
}

//! ---o
void Node::doDestroy() {
	removeFromParent();
	removeStates();
	relTransformation.reset();
	setStatus(STATUS_HAS_SRT,false);
	worldLocation.reset();
	removeAttributes();
}


void Node::destroy() {
	std::deque<Node::ref_t> nodes; // keep references during the destruction process (can crash otherwise).
	
	// collect and mark as destroyed
	MinSG::forEachNodeTopDown( this, [&nodes](Node * node){
		nodes.emplace_back(node);
		node->setStatus( STATUS_IS_DESTROYED, true ); // set before dissolving the tree
		node->setStatus( STATUS_CONTAINS_NODE_REMOVED_OBSERVER,false); // prevent observers to be triggered
	});

	// remove data and dissolve tree
	for(const auto & node : nodes)
		node->doDestroy();
}

//! (internal)
void Node::updateObservedStatus(){
	
	MinSG::traverseTopDown(this,[](Node * node){
		const statusFlag_t initialStatus = node->statusFlags;
		const Node * parent = node->getParent();

		// enable observation if the node has an observer or if its parent is under observation
		node->setStatus(STATUS_TRANSFORMATION_OBSERVED,	(parent && parent->getStatus(STATUS_TRANSFORMATION_OBSERVED)) || 
															node->getStatus(STATUS_CONTAINS_TRANSFORMATION_OBSERVER) );


		node->setStatus(STATUS_TREE_OBSERVED,	(parent && parent->getStatus(STATUS_TREE_OBSERVED)) || 
															node->getStatus(STATUS_CONTAINS_NODE_ADDED_OBSERVER) || 
															node->getStatus(STATUS_CONTAINS_NODE_REMOVED_OBSERVER) );

		// continue only if something changed
		return (node->statusFlags==initialStatus) ? NodeVisitor::BREAK_TRAVERSAL : NodeVisitor::CONTINUE_TRAVERSAL;
	});
}


// -----------------------------------
// ---- Clones and Instances

//! (internal) it is assumed, that the structure of the given subtrees tree is identical
static void traverseTrees(const Node* n1,const Node* n2,std::function<void (Node*,Node*)> fun){
	auto nodes1 = collectNodes<Node>(n1);
	auto nodes2 = collectNodes<Node>(n2);
	FAIL_IF( nodes1.size()!=nodes2.size() );

	auto i1 = nodes1.begin();
	auto i2 = nodes2.begin();
	for(;i1!=nodes1.end();++i1,++i2){
		FAIL_IF((*i1)->getTypeId()!=(*i2)->getTypeId());
		fun(*i1,*i2);
	}

}

Node * Node::clone(bool cloneAttributes) const {
	Node * clonedNode = doClone(); // create a clone without attributes
	if(!clonedNode)
		throw std::logic_error("Node could not be cloned '"+std::string(getTypeName())+"'");

	if(cloneAttributes){
		// copy attributes marked as NodeAttributeKey::COPY_TO_CLONE
		traverseTrees(this, clonedNode, [](Node* originalNode, Node* newNode){
			if(originalNode->hasAttributes()){
				for(const auto & entry : *(originalNode->getAttributes())) {
					if(entry.second && NodeAttributeModifier::isCopiedToClone(entry.first)) {
						newNode->setAttribute(entry.first, entry.second->clone());
					}
				}
			}
		});
	}
	
	return clonedNode;
}

//! (static)
Node * Node::createInstance(const Node * source){
	if(source==nullptr){
		return nullptr;
	}else if(source->isInstance()){
		return source->clone();
	}
	Util::Reference<Node> instance = source->clone(false); // create a clone without attributes
	
	// copy attributes marked as NodeAttributeKey::COPY_TO_INSTANCE
	traverseTrees(source, instance.get(), [](Node* originalNode, Node* newNode){
		newNode->_setPrototype( originalNode->isInstance() ? originalNode->getPrototype() : originalNode);
		if(originalNode->hasAttributes()){
			for(const auto & entry : *(originalNode->getAttributes())) {
				if(entry.second && NodeAttributeModifier::isCopiedToInstance(entry.first))
					newNode->setAttribute(entry.first, entry.second->clone());
			}
		}
	});

	return instance.detachAndDecrease();
}


static Util::StringIdentifier attrName_prototype(NodeAttributeModifier::create("NodePrototype", NodeAttributeModifier::PRIVATE_ATTRIBUTE)); // don't save, copy to clones or to instances
typedef Util::ReferenceAttribute<Node> prototypeContainer_t;


void Node::_setPrototype(Node * n){
	if(n==nullptr){
		setStatus(STATUS_IS_INSTANCE,false);
		unsetAttribute(attrName_prototype);
	}else{
		setStatus(STATUS_IS_INSTANCE,true);
		setAttribute(attrName_prototype,new prototypeContainer_t(n));
	}
}

		
Node * Node::getPrototype()const{
	Node * prototype = nullptr;
	if(isInstance()){
		const auto attr = dynamic_cast<prototypeContainer_t*>(getAttribute(attrName_prototype));
		if(attr!=nullptr){
			prototype = attr->get();
		}
	}
	return prototype;
}

Util::GenericAttribute * Node::findAttribute(const Util::StringIdentifier & key)const{
	auto attr = getAttribute(key);
	if(attr==nullptr){
		Node * prototype = getPrototype();
		if(prototype!=nullptr){
			attr = prototype->getAttribute(key);
		}
	}
	return attr;
}

// -----------------------------------
// ---- Matrix

const Geometry::Matrix4x4* Node::getMatrixPtr() const {
	if(hasSRT() && !getStatus(STATUS_MATRIX_REFLECTS_SRT)){ // matrix is invalid
		relTransformation->matrix = Geometry::Matrix4x4(accessSRT());
		setStatus(STATUS_MATRIX_REFLECTS_SRT,true);
	}
	return relTransformation ? &(relTransformation->matrix) : nullptr;
}

const Geometry::Matrix4x4& Node::getMatrix() const {
	const Geometry::Matrix4x4 *m = getMatrixPtr();
	static const Geometry::Matrix4x4 nullTransform;
	return m ? *m : nullTransform;
}

void Node::setMatrix(const Geometry::Matrix4x4 & m) {
	if(!relTransformation)
		relTransformation.reset( new RelativeTransformation );
	relTransformation->matrix = m;
	setStatus(STATUS_HAS_SRT,false);
	transformationChanged();
}


void Node::reset() {
	setStatus(STATUS_HAS_SRT,false);
	relTransformation.reset();
	worldLocation.reset();
	transformationChanged();
}

// -----------------------------------
// ---- SRT

const Geometry::SRT& Node::getSRT() const {
	if(hasSRT())
		return relTransformation->srt;
	else if(hasMatrix())
		return accessSRT(); 
	static const Geometry::SRT nullTransform;
	return nullTransform;
}

void Node::setSRT(const Geometry::SRT & newSRT) {
	if(!relTransformation)
		relTransformation.reset( new RelativeTransformation );
	relTransformation->srt = newSRT;
	setStatus(STATUS_HAS_SRT,true);
	transformationChanged();
}

Geometry::SRT & Node::accessSRT() const {
	if(!hasSRT()) {
		if(relTransformation){ // == hasMatrix
			if( !relTransformation->matrix.convertsSafelyToSRT() )
				WARN("Dirty convert of matrix to SRT");
			relTransformation->srt = relTransformation->matrix._toSRT();
			relTransformation->matrix = Geometry::Matrix4x4(relTransformation->srt);
			setStatus(STATUS_MATRIX_REFLECTS_SRT,true);
		} else {
			relTransformation.reset( new RelativeTransformation );
		}
		setStatus(STATUS_HAS_SRT,true);
	}
	return relTransformation->srt;
}


// -----------------------------------
// ---- States

void Node::removeStates() {
	states.reset();
}

void Node::addState(State * sn) {
	if(sn) {
		if(!states) 
			states.reset(new stateList_t);
		states->emplace_back(sn, false);
	}
}

void Node::removeState(State * sn) {
	if(hasStates() && sn){
		states->erase(std::remove_if(states->begin(), states->end(),
							   [&sn](const std::pair<Util::Reference<State>, bool> & entry) {
									return entry.first == sn;
							   }),
					states->end());
	}
}
std::vector<State*> Node::getStates()const{
	std::vector<State*> states2;
	if(states){
		for(const auto & stateEntry : *states)
			states2.push_back(stateEntry.first.get());
	}
	return states2;
}

// -----------------------------------
// ---- Transformations

void Node::lookAtAbs(const Geometry::Vec3 & pointInWorldCoordinates){
	rotateToWorldDir(pointInWorldCoordinates-getWorldOrigin());
}

void Node::rotateToWorldDir(const Geometry::Vec3 & directionInWorldCoordinates){

	//! \todo this function may call transformationChanged() up to three times! Better first calculate new SRT and the simply set it once...
	accessSRT().resetRotation(); // reset old rotations (makes things more stable, but removes z-rotations)
	transformationChanged();

	const Geometry::Vec3 current(0,0,1);
	{ // rotate around y
		const Geometry::Vec3 target=getWorldMatrix().inverse().transformPosition(directionInWorldCoordinates+getWorldOrigin());

		Geometry::Vec3 a(target.x(),0,target.z());
		Geometry::Vec3 b(current.x(),0,current.z());
		if ( !a.isZero() && !b.isZero() ){
			a.normalize();
			b.normalize();
			rotateRel_rad( acos(a.dot(b))* (a.dot(Geometry::Vec3(1,0,0))>0?1:-1) , Geometry::Vec3(0,1,0) );
		}
	}
	{ // rotate around x
		const Geometry::Vec3 target=getWorldMatrix().inverse().transformPosition(directionInWorldCoordinates+getWorldOrigin());

		Geometry::Vec3 a(0,target.y(),target.z());
		Geometry::Vec3 b(0,current.y(),current.z());
		if ( !a.isZero() && !b.isZero() ){
			a.normalize();
			b.normalize();
			rotateLocal_rad( acos(a.dot(b)) * (a.dot(Geometry::Vec3(0,1,0))<0?1:-1), Geometry::Vec3(1,0,0) );
		}
	}

//	
}

void Node::setWorldOrigin(const Geometry::Vec3 & v) {
	const Geometry::Matrix4x4 * parentWorldMatrix = hasParent() ? getParent()->getWorldMatrixPtr() : nullptr;
	setRelPosition( parentWorldMatrix==nullptr ? v : parentWorldMatrix->inverse().transformPosition(v) );
}

// -----------------------------------
// ---- Observer
static Util::StringIdentifier attrName_transformationObservers( NodeAttributeModifier::create("transformationObservers", NodeAttributeModifier::PRIVATE_ATTRIBUTE) );
static Util::StringIdentifier attrName_nodeAddedObservers( 		NodeAttributeModifier::create("nodeAddedObservers", NodeAttributeModifier::PRIVATE_ATTRIBUTE) );
static Util::StringIdentifier attrName_nodeRemovedObservers( 	NodeAttributeModifier::create("nodeRemovedObservers", NodeAttributeModifier::PRIVATE_ATTRIBUTE) );

typedef Util::WrapperAttribute<std::vector<Node::transformationObserverFunc>> transformationObserversContainer_t;
typedef Util::WrapperAttribute<std::vector<Node::nodeAddedObserverFunc>> nodeAddedObserversContainer_t;
typedef Util::WrapperAttribute<std::vector<Node::nodeRemovedObserverFunc>> nodeRemovedObserversContainer_t;


void Node::transformationChanged() {
	setStatus(STATUS_MATRIX_REFLECTS_SRT, false);
	invalidateWorldMatrix();
	worldBBChanged();

	for(Node * n = this; n!=nullptr&&n->isTransformationObserved(); n = n->getParent()){
		if(n->getStatus(STATUS_CONTAINS_TRANSFORMATION_OBSERVER)){
			auto observers = dynamic_cast<transformationObserversContainer_t*>(n->getAttribute(attrName_transformationObservers));
			if(observers==nullptr){
				std::cout<<"No Observer found in node "<<n<<"\n";
				std::cout<<"Attribute is "<<n->getAttribute(attrName_transformationObservers)<<"\n";
				continue;
			}
			for(auto & observer : **observers)
				observer(this);
		}
	}
}
void Node::informNodeAddedObservers(Node * addedNode){
	for(Node * n = this; n!=nullptr&&n->getStatus(STATUS_TREE_OBSERVED); n = n->getParent()){
		if(n->getStatus(STATUS_CONTAINS_NODE_ADDED_OBSERVER)){
			auto observers = dynamic_cast<nodeAddedObserversContainer_t*>(n->getAttribute(attrName_nodeAddedObservers));
			if(observers){
				for(auto & observer:**observers)
					observer( addedNode );
			}
		}
	}
}
void Node::informNodeRemovedObservers(GroupNode * parent, Node * removedNode){
	for(Node * n = this; n!=nullptr&&n->getStatus(STATUS_TREE_OBSERVED); n = n->getParent()){
		if(n->getStatus(STATUS_CONTAINS_NODE_REMOVED_OBSERVER)){
			auto observers = dynamic_cast<nodeRemovedObserversContainer_t*>(n->getAttribute(attrName_nodeRemovedObservers));
			if(observers){
				for(auto & observer:**observers)
					observer( parent, removedNode );
			}
		}
	}
}

void Node::addTransformationObserver(const transformationObserverFunc & func){
	auto observers = dynamic_cast<transformationObserversContainer_t*>(getAttribute(attrName_transformationObservers));
	if(observers==nullptr){
		observers = new transformationObserversContainer_t;
		setAttribute(attrName_transformationObservers,observers);
	}
	observers->ref().push_back(func);
	setStatus(STATUS_CONTAINS_TRANSFORMATION_OBSERVER,true);
	updateObservedStatus();
}
void Node::addNodeAddedObserver(const nodeAddedObserverFunc & func){
	auto observers = dynamic_cast<nodeAddedObserversContainer_t*>(getAttribute(attrName_nodeAddedObservers));
	if(observers==nullptr){
		observers = new nodeAddedObserversContainer_t;
		setAttribute(attrName_nodeAddedObservers,observers);
	}
	observers->ref().push_back(func);
	setStatus(STATUS_CONTAINS_NODE_ADDED_OBSERVER,true);
	updateObservedStatus();
}
void Node::addNodeRemovedObserver(const nodeRemovedObserverFunc & func){
	auto observers = dynamic_cast<nodeRemovedObserversContainer_t*>(getAttribute(attrName_nodeRemovedObservers));
	if(observers==nullptr){
		observers = new nodeRemovedObserversContainer_t;
		setAttribute(attrName_nodeRemovedObservers,observers);
	}
	observers->ref().push_back(func);
	setStatus(STATUS_CONTAINS_NODE_REMOVED_OBSERVER,true);
	updateObservedStatus();
}
void Node::clearTransformationObservers(){
	unsetAttribute(attrName_transformationObservers);
	setStatus(STATUS_CONTAINS_TRANSFORMATION_OBSERVER,false);
	updateObservedStatus();
}
void Node::clearNodeAddedObservers(){
	unsetAttribute(attrName_nodeAddedObservers);
	setStatus(STATUS_CONTAINS_NODE_ADDED_OBSERVER,false);
	updateObservedStatus();
}
void Node::clearNodeRemovedObservers(){
	unsetAttribute(attrName_nodeRemovedObservers);
	setStatus(STATUS_CONTAINS_NODE_REMOVED_OBSERVER,false);
	updateObservedStatus();
}


// -----------------------------------
// ---- Traversal

//! ---o
NodeVisitor::status Node::traverse(NodeVisitor & visitor) {
	NodeVisitor::status status = visitor.enter(this);
	return status == NodeVisitor::EXIT_TRAVERSAL ? status : visitor.leave(this);
}

// -----------------------------------
// ---- Information

size_t Node::getMemoryUsage() const {
	size_t size = sizeof(Node);
	if(hasAttributes()) {
		size += sizeof(Util::GenericAttributeMap) + getAttributes()->size() * sizeof(Util::GenericAttribute);
	}
	if(worldLocation) {
		size += sizeof(worldLocation);
	}
	if(states) {
		size += sizeof(stateList_t) + states->size() * sizeof(stateList_t::value_type);
		for(const auto & stateStatusPair : *states) {
			if(stateStatusPair.first.isNotNull()) {
				size += sizeof(State);
			}
		}
	}
	if(relTransformation) {
		size += sizeof(RelativeTransformation);
	}
	return size;
}


// -----------------------------------
// ---- WorldLocation

const Geometry::Box& Node::getWorldBB() const {
	if(!getStatus(STATUS_WORLD_BB_VALID)) {
		// if the node is not transformed -> delete the worldBB and the world matrix
		if(!hasMatrix() && ( getWorldMatrixPtr()==nullptr || getWorldMatrixPtr()->isIdentity() ) ){
			worldLocation.reset();
			setStatus(STATUS_WORLD_MATRIX_VALID, true); // this also validates the world matrix
		} else { // node is transformed 
			if(!worldLocation)	// no worldLocation -> create a new worldLocation
				worldLocation.reset( new WorldLocation );
			worldLocation->worldBB = Geometry::Helper::getTransformedBox(getBB(), getWorldMatrix());
		} 
		setStatus(STATUS_WORLD_BB_VALID, true); // \note the world matrix is now also valid.
	}
	return worldLocation ? worldLocation->worldBB : getBB();
}

const Geometry::Matrix4x4& Node::getWorldMatrix() const {
	static const Geometry::Matrix4x4 nullTransform;
	const Geometry::Matrix4x4* ptr = getWorldMatrixPtr();
	return ptr ? *ptr : nullTransform;
}

const Geometry::Matrix4x4* Node::getWorldMatrixPtr() const {
	if(!getStatus(STATUS_WORLD_MATRIX_VALID)) {
		const Geometry::Matrix4x4 * localMatrix = getMatrixPtr();
		const Geometry::Matrix4x4 * parentWorldMatrix = hasParent() ? parentNode->getWorldMatrixPtr() : nullptr;

		if(localMatrix || parentWorldMatrix ){ // node is transformed
			if(!worldLocation)	// no worldLocation -> create a new worldLocation
				worldLocation.reset( new WorldLocation );
			if(localMatrix && parentWorldMatrix){
				worldLocation->worldMatrix = (*parentWorldMatrix) * (*localMatrix);
			}else if(parentWorldMatrix){ // && !localMatrix
				worldLocation->worldMatrix = (*parentWorldMatrix);
			}else{ // !parentWorldMatrix && localMatrix
				worldLocation->worldMatrix = (*localMatrix);
			}
		}else{ // node is not transformed -> delete the worldBB and the world matrix
			worldLocation.reset();
			setStatus(STATUS_WORLD_BB_VALID, true); // this also validates the world bb
		}
		setStatus(STATUS_WORLD_MATRIX_VALID, true);
	}
	return worldLocation ? &(worldLocation->worldMatrix) : nullptr;
}

void Node::invalidateWorldMatrix() const {
	traverseTopDown(const_cast<Node*>(this),[](Node* node){
		if(node->getStatus(STATUS_WORLD_MATRIX_VALID)){
			node->setStatus(STATUS_WORLD_MATRIX_VALID,false);
			node->setStatus(STATUS_WORLD_BB_VALID,false);
			return NodeVisitor::CONTINUE_TRAVERSAL;
		}else{
			return NodeVisitor::BREAK_TRAVERSAL;

		}
	});
}


}


