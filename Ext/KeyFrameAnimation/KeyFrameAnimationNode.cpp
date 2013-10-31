/*
	This file is part of the MinSG library extension KeyFrameAnimation.
	Copyright (C) 2010-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2010-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2010 David Maicher
	Copyright (C) 2010-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "KeyFrameAnimationNode.h"
#include <Geometry/Interpolation.h>
#include <Rendering/Mesh/MeshDataStrategy.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Util/Macros.h>

namespace MinSG {

const short KeyFrameAnimationNode::STATE_LOOP_MODE = 0;
const short KeyFrameAnimationNode::STATE_SINGLE_MODE = 1;
const short KeyFrameAnimationNode::STATE_SINGLE_MODE_FINISHED = 2;

KeyFrameAnimationNode::KeyFrameAnimationNode(const Rendering::MeshIndexData & indexData, const std::vector<Rendering::MeshVertexData> & framesData,
		const std::map<std::string, std::vector<int> > animationData):GeometryNode(){

	keyFrameAnimationData = new KeyFrameAnimationData(indexData, framesData, animationData);
	keyFrameAnimationBehaviour = nullptr;

	vertexDescription.appendPosition3D();
	vertexDescription.appendNormalFloat();
	vertexDescription.appendTexCoord();
	if(!framesData.empty() && !(vertexDescription == framesData[0].getVertexDescription() )){
		WARN("KeyFrameAnimationData needs well defined vertex format");
		FAIL();
	}

	curTime = 0;
	speedFactor = 1.0;
	lastTimeStamp = -1;

	curAnimationPosition = 0.0;

	curState = KeyFrameAnimationNode::STATE_LOOP_MODE;

	activeAnimation = std::make_pair("", std::vector<int>());

	//try to activate 'run' animation
	if(!setActiveAnimation("run")){
		if(!animationData.empty()){
			//take first animation
			auto it = animationData.begin();
			activeAnimation = make_pair(it->first, it->second);
		}else{
			//no animations!
			WARN("no animations found");
			return;
		}
	}

	setMesh(createInitialMesh());
}

KeyFrameAnimationNode::KeyFrameAnimationNode(const KeyFrameAnimationNode & source, Rendering::VertexDescription _vertexDescription,
		std::pair<std::string, std::vector<int> > _activeAnimation, float _curAnimationPosition, float _curTime,
		float _speedFactor, float _lastTimeStamp, short _curState) : GeometryNode(source),
		keyFrameAnimationData(source.keyFrameAnimationData), keyFrameAnimationBehaviour(nullptr){

	vertexDescription = _vertexDescription;

	curTime = _curTime;
	speedFactor = _speedFactor;
	lastTimeStamp = _lastTimeStamp;

	curAnimationPosition = _curAnimationPosition;

	curState = _curState;

	activeAnimation = _activeAnimation;

	setMesh(createInitialMesh());
}

KeyFrameAnimationNode::~KeyFrameAnimationNode() = default;

//! ---|> [GeometryNode]
KeyFrameAnimationNode * KeyFrameAnimationNode::doClone() const {
	return new KeyFrameAnimationNode(*this, vertexDescription, activeAnimation, curAnimationPosition,
			curTime, speedFactor, lastTimeStamp, curState);
}

Rendering::Mesh * KeyFrameAnimationNode::createInitialMesh() const {
	Rendering::MeshVertexData vertexData;
	setVertexData(vertexData, activeAnimation.second[0], activeAnimation.second[0], 0.0f);;

	auto animationMesh = new Rendering::Mesh(keyFrameAnimationData->getIndexData(), vertexData);
	animationMesh->setDataStrategy(Rendering::SimpleMeshDataStrategy::getPureLocalStrategy());
	return animationMesh;
}

void KeyFrameAnimationNode::setVertexData(Rendering::MeshVertexData & vertexData, int startFrameIndex, int endFrameIndex, float interpolatePercentage) const {
	const Rendering::MeshVertexData & startData = keyFrameAnimationData->getFramesData()[startFrameIndex];
	const Rendering::MeshVertexData & endData = keyFrameAnimationData->getFramesData()[endFrameIndex];

	vertexData.allocate(startData.getVertexCount(), vertexDescription);

	float * const outputData = reinterpret_cast<float *>(vertexData.data());
	const float * const vertexDataStart = reinterpret_cast<const float * const>(startData.data());
	const float * const vertexDataEnd = reinterpret_cast<const float * const>(endData.data());

	const uint32_t vertexCount = 8 * startData.getVertexCount();
	for (uint_fast32_t i = 0; i < vertexCount; i += 8) {
		// VERTEX
		outputData[i + 0] = Geometry::Interpolation::linear(vertexDataStart[i + 0], vertexDataEnd[i + 0], interpolatePercentage);
		outputData[i + 1] = Geometry::Interpolation::linear(vertexDataStart[i + 1], vertexDataEnd[i + 1], interpolatePercentage);
		outputData[i + 2] = Geometry::Interpolation::linear(vertexDataStart[i + 2], vertexDataEnd[i + 2], interpolatePercentage);
		// NORMAL
		outputData[i + 3] = Geometry::Interpolation::linear(vertexDataStart[i + 3], vertexDataEnd[i + 3], interpolatePercentage);
		outputData[i + 4] = Geometry::Interpolation::linear(vertexDataStart[i + 4], vertexDataEnd[i + 4], interpolatePercentage);
		outputData[i + 5] = Geometry::Interpolation::linear(vertexDataStart[i + 5], vertexDataEnd[i + 5], interpolatePercentage);
		// TEX0
		outputData[i + 6] = Geometry::Interpolation::linear(vertexDataStart[i + 6], vertexDataEnd[i + 6], interpolatePercentage);
		outputData[i + 7] = Geometry::Interpolation::linear(vertexDataStart[i + 7], vertexDataEnd[i + 7], interpolatePercentage);
	}

	vertexData.updateBoundingBox();
}

bool KeyFrameAnimationNode::updateMesh(float timeStampSec){
	if(getMesh() != nullptr && !activeAnimation.first.empty()){

		if (lastTimeStamp == -1) {
			lastTimeStamp = timeStampSec;
		}

		float numFrames = activeAnimation.second[1] - activeAnimation.second[0] + 1;
		float animationTime = numFrames / static_cast<float>(activeAnimation.second[2] * speedFactor);

		curTime += timeStampSec-lastTimeStamp;
		if (curTime > animationTime) {
			if(curState == KeyFrameAnimationNode::STATE_LOOP_MODE){
				curTime = curTime - animationTime;
			}
			else{
				curTime = 0;
				curState = KeyFrameAnimationNode::STATE_SINGLE_MODE_FINISHED;
			}
		}

		short startFrame = 0;
		short endFrame = 0;
		float interpolatePercentage = 0.0;

		//if startframe != endframe
		if(activeAnimation.second[0] != activeAnimation.second[1] && (curState == KeyFrameAnimationNode::STATE_LOOP_MODE || curState == KeyFrameAnimationNode::STATE_SINGLE_MODE)){
			startFrame = static_cast<short>(curTime / animationTime * numFrames);
			endFrame= (startFrame + 1) % static_cast<short>(numFrames);
			interpolatePercentage = curTime / animationTime * numFrames - startFrame;
		}

		setVertexData(getMesh()->openVertexData(), activeAnimation.second[0] + startFrame, activeAnimation.second[0] + endFrame, interpolatePercentage);

		lastTimeStamp = timeStampSec;
		worldBBChanged();
		return true;
	}
	return false;
}

bool KeyFrameAnimationNode::setActiveAnimation(const std::string & name){
	const std::map<std::string, std::vector<int> >::const_iterator it = keyFrameAnimationData->getAnimationData().find(name);
	if (it != keyFrameAnimationData->getAnimationData().end()) {
		activeAnimation = make_pair(it->first, it->second);
		curTime = 0;
		return true;
	}
	return false;
}

bool KeyFrameAnimationNode::setSpeedFactor(const float & sf){
	if(sf >= 0){
		speedFactor = sf;
		return true;
	}
	return false;
}

void KeyFrameAnimationNode::setState(const short & value){
	curState = value;
	curTime = 0;
}

short KeyFrameAnimationNode::getState(){
	return curState;
}

bool KeyFrameAnimationNode::setAnimationPosition(const float & value){
	if(value < 0) return false;

	curAnimationPosition = value - static_cast<int>(value);

	float numFrames = activeAnimation.second[1] - activeAnimation.second[0] + 1;

	short startFrame = static_cast<short>(curAnimationPosition * numFrames);
	short endFrame = (startFrame + 1) % static_cast<short>(numFrames);

	float interpolatePercentage = curAnimationPosition * numFrames - startFrame;

	setVertexData(getMesh()->openVertexData(), activeAnimation.second[0] + startFrame, activeAnimation.second[0] + endFrame, interpolatePercentage);
		worldBBChanged();
	return true;
}

float KeyFrameAnimationNode::getAnimationPosition(){
	return curAnimationPosition;
}


std::map<std::string, std::vector<int> > KeyFrameAnimationNode::getAnimationData() {
	return keyFrameAnimationData->getAnimationData();
}

void KeyFrameAnimationNode::setBehaviour(KeyFrameAnimationBehaviour * b){
	keyFrameAnimationBehaviour = b;
}

KeyFrameAnimationBehaviour * KeyFrameAnimationNode::getBehaviour(){
	return keyFrameAnimationBehaviour;
}

}
