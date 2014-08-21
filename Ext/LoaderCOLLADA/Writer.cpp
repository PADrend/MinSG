/*
    This file is part of the MinSG library extension LoaderCOLLADA.
    Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
    Copyright (C) 2012 Lukas Kopecki

    This library is subject to the terms of the Mozilla Public License, v. 2.0.
    You should have received a copy of the MPL along with this library; see the
    file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_LOADERCOLLADA

#include "Writer.h"
#include "Utils/DescriptionUtils.h"
#include "Externals/ExtImporters.h"

#include "Core/NodeImporter.h"
#include "Core/VisualSceneImporter.h"
#include "Core/StateImporter.h"
#include "Core/FileDescriptionImporter.h"

#include "../../SceneManagement/SceneDescription.h"

namespace MinSG {
namespace LoaderCOLLADA {

Writer::Writer() : IWriter(), sceneDescription(), invertTransparency(false) {

	// Core functionalities
	setGeometryFunction(LoaderCOLLADA::nodeCoreImporter);
	setVisualSceneFunction(LoaderCOLLADA::visualSceneCoreImporter);
	setEffectFunction(std::bind(LoaderCOLLADA::effectCoreImporter, std::placeholders::_1, std::placeholders::_2, std::ref(invertTransparency)));
	setMaterialFunction(LoaderCOLLADA::materialCoreImporter);
	setLightFunction(LoaderCOLLADA::lightCoreImporter);
	setImageFunction(LoaderCOLLADA::imageCoreImporter);
	setFileInformationFunction(LoaderCOLLADA::fileInformationImporter);
	setSceneNodeFunction(LoaderCOLLADA::sceneNodeImporter);

	// External functions
	ExternalImporter::initExternalFunctions(*this);
}

Writer::~Writer() = default;

Writer & Writer::instance() {
	static Writer writer;
	return writer;
}

void Writer::beginLoadingProcess() {
	// TODO: Idea: put the required data in a context object valid for one loading process (RAII)
	sceneDescription = new SceneManagement::DescriptionMap;

	sceneDescription->setString(SceneManagement::Consts::TYPE, SceneManagement::Consts::TYPE_SCENE);

	scene = new SceneManagement::DescriptionMap;
	scene->setString(SceneManagement::Consts::TYPE, SceneManagement::Consts::TYPE_NODE);
	scene->setString(SceneManagement::Consts::ATTR_NODE_TYPE, SceneManagement::Consts::NODE_TYPE_LIST);
	addToMinSGChildren(sceneDescription, scene);

	referenceRegistry.clear();
}

void Writer::endLoadingProcess() {
	referenceRegistry.clear();
}

/*
 *  Core functions
 */
bool Writer::writeGeometry(const COLLADAFW::Geometry * geometry) {
	return geometryFunc(geometry, referenceRegistry);
}

bool Writer::writeVisualScene(const COLLADAFW::VisualScene * visualScene) {
	return visualSceneFunc(visualScene, referenceRegistry, scene, sceneNodeFunc);
}

bool Writer::writeMaterial(const COLLADAFW::Material * material) {
	return materialFunc(material, referenceRegistry);
}

bool Writer::writeEffect(const COLLADAFW::Effect * effect) {
	return effectFunc(effect, referenceRegistry);
}

bool Writer::writeLight(const COLLADAFW::Light * light) {
	return lightFunc(light, referenceRegistry);
}

bool Writer::writeImage(const COLLADAFW::Image * image) {
	return imageFunc(image, referenceRegistry);
}

bool Writer::writeGlobalAsset(const COLLADAFW::FileInfo * asset) {
	copyAttributesToNodeDescription(scene, fileInformationFunc(asset, referenceRegistry));
	return true;
}

/*
 *  External functions
 */
bool Writer::writeController(const COLLADAFW::Controller * controller) {
	return controlerFunc(controller, referenceRegistry);
}

bool Writer::writeSkinControllerData(const COLLADAFW::SkinControllerData * skinControllerData) {
	return skinControllerFunc(skinControllerData, referenceRegistry);
}

bool Writer::writeAnimation(const COLLADAFW::Animation * animation) {
	return animationFunc(animation, referenceRegistry);
}

bool Writer::writeAnimationList(const COLLADAFW::AnimationList * animationList) {
	return animationListFunc(animationList, referenceRegistry);
}

}
}

#endif /* MINSG_EXT_LOADERCOLLADA */
