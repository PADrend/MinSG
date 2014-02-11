/*
    This file is part of the MinSG library extension LoaderCOLLADA.
    Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
    Copyright (C) 2012 Lukas Kopecki

    This library is subject to the terms of the Mozilla Public License, v. 2.0.
    You should have received a copy of the MPL along with this library; see the
    file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_LOADERCOLLADA

#ifndef MINSG_LOADERCOLLADA_WRITER_H
#define MINSG_LOADERCOLLADA_WRITER_H

/**
 * @file
 * This header file uses the OpenCOLLADA library directly. Because the
 * external library should not be exposed by the MinSG API, this header file
 * must not be included from outside of the LoaderCOLLADA extension.
 */

#include "Utils/LoaderCOLLADAConsts.h"

#include <COLLADAFWIWriter.h>

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace COLLADAFW {
class Animation;
class AnimationList;
class Camera;
class Controller;
class Effect;
class FileInfo;
class Formula;
class Formulas;
class Geometry;
class Image;
class KinematicsScene;
class LibraryNodes;
class Light;
class Material;
class Scene;
class SkinControllerData;
typedef std::string String;
class VisualScene;
}

namespace Util {
class GenericAttributeMap;
}
namespace MinSG {
namespace SceneManagement {
typedef Util::GenericAttributeMap NodeDescription;
}
namespace LoaderCOLLADA {

class Writer : public COLLADAFW::IWriter {
		//! @name Internal data for loading process
		//@{
	private:
		referenceRegistry_t referenceRegistry;

		SceneManagement::NodeDescription * sceneDescription; // Metadata for scene description, root of all
		SceneManagement::NodeDescription * scene;           // scene information, geometries, states ...   added for faster lookup (child of sceneDescription)
		//@}

		//! @name Singleton definitions
		//@{
	public:
		static Writer & instance();

	private:
		Writer();
		virtual ~Writer();
		//@}

		//! @name Core loading routines
		//@{
	public:
		typedef std::function<bool (const COLLADAFW::Geometry *, referenceRegistry_t &)> geometryFunc_t;
		typedef std::function<bool (const COLLADAFW::Material *, referenceRegistry_t &)> materialFunc_t;
		typedef std::function<bool (const COLLADAFW::Effect *, referenceRegistry_t &)> effectFunc_t;
		typedef std::function<bool (const COLLADAFW::Light *, referenceRegistry_t &)> lightFunc_t;
		typedef std::function<bool (const COLLADAFW::Image *, referenceRegistry_t &)> imageFunc_t;

		typedef std::function<SceneManagement::NodeDescription *(const COLLADAFW::FileInfo *, referenceRegistry_t &)> fileInformationFunc_t;
		typedef std::function<bool (const COLLADAFW::VisualScene *, referenceRegistry_t &, SceneManagement::NodeDescription *, const sceneNodeFunc_t &)> visualSceneFunc_t;

		void setGeometryFunction(const geometryFunc_t & func) {
			geometryFunc = func;
		}
		void setMaterialFunction(const materialFunc_t & func) {
			materialFunc = func;
		}
		void setEffectFunction(const effectFunc_t & func) {
			effectFunc = func;
		}
		void setLightFunction(const lightFunc_t & func) {
			lightFunc = func;
		}
		void setImageFunction(const imageFunc_t & func) {
			imageFunc = func;
		}
		void setFileInformationFunction(const fileInformationFunc_t & func) {
			fileInformationFunc = func;
		}
		void setSceneNodeFunction(const sceneNodeFunc_t & func) {
			sceneNodeFunc = func;
		}
		void setVisualSceneFunction(const visualSceneFunc_t & func) {
			visualSceneFunc = func;
		}
	private:
		geometryFunc_t geometryFunc;
		materialFunc_t materialFunc;
		effectFunc_t effectFunc;
		lightFunc_t lightFunc;
		imageFunc_t imageFunc;
		fileInformationFunc_t fileInformationFunc;
		sceneNodeFunc_t sceneNodeFunc;
		visualSceneFunc_t visualSceneFunc;
		//@}

		//! @name Ext loading routines
		//@{
	public:
		typedef std::function<bool (const COLLADAFW::Controller *, referenceRegistry_t &)> controllerFunc_t;
		typedef std::function<bool (const COLLADAFW::SkinControllerData *, referenceRegistry_t &)> skinControllerFunc_t;
		typedef std::function<bool (const COLLADAFW::Animation *, referenceRegistry_t &)> animationFunc_t;
		typedef std::function<bool (const COLLADAFW::AnimationList *, referenceRegistry_t &)> animationListFunc_t;

		void setControllerFunction(const controllerFunc_t & func) {
			controlerFunc = func;
		}
		void setSkinControllerFunction(const skinControllerFunc_t & func) {
			skinControllerFunc = func;
		}
		void setAnimationFunction(const animationFunc_t & func) {
			animationFunc = func;
		}
		void setAnimationListFunction(const animationListFunc_t & func) {
			animationListFunc = func;
		}
	private:
		controllerFunc_t controlerFunc;
		skinControllerFunc_t skinControllerFunc;
		animationFunc_t animationFunc;
		animationListFunc_t animationListFunc;
		//@}

		//! @name Loading flags
		//@{
	private:
		bool invertTransparency;

	public:
		void setInvertTransparency(bool value) {
			invertTransparency = value;
		}
		//@}

		//! @name Loading interface
		//@{
	public:
		/**
		 * Inform the writer that a new loading process should be started and
		 * let it allocate data that is needed.
		 */
		void beginLoadingProcess();

		//! Retrieve the scene description and take ownership of the pointer.
		SceneManagement::NodeDescription * releaseSceneDescription() {
			return sceneDescription;
		}

		/**
		 * Inform the writer that the loading process is finished and any
		 * allocated data can be deleted.
		 */
		void endLoadingProcess();
		//@}

		//! @name Implementation of COLLADAFW::IWriter interface
		//@{
	public:
		/** This method will be called if an error in the loading process occurred and the loader cannot
		 continue to to load. The writer should undo all operations that have been performed.
		 @param errorMessage A message containing information about the error that occurred.
		 */
		void cancel(const COLLADAFW::String & /*errorMessage*/) override {
		}

		/** This is the method called. The writer hast to prepare to receive data.*/
		void start() override {
		}

		/** This method is called after the last write* method. No other methods will be called after this.*/
		void finish() override {
		}

		/** When this method is called, the writer must write the global document asset.
		 @return The writer should return true, if writing succeeded, false otherwise.*/
		bool writeGlobalAsset(const COLLADAFW::FileInfo * asset) override;

		/** When this method is called, the writer must write the entire visual scene.
		 @return The writer should return true, if writing succeeded, false otherwise.*/
		bool writeScene(const COLLADAFW::Scene * /*scene*/) override {
			return true;
		}

		/** When this method is called, the writer must write the entire visual scene.
		 @return The writer should return true, if writing succeeded, false otherwise.*/
		bool writeVisualScene(const COLLADAFW::VisualScene * visualScene) override;

		/** When this method is called, the writer must handle all nodes contained in the
		 library nodes.
		 @return The writer should return true, if writing succeeded, false otherwise.*/
		bool writeLibraryNodes(const COLLADAFW::LibraryNodes * /*libraryNodes*/) override {
			return true;
		}

		/** When this method is called, the writer must write the geometry.
		 @return The writer should return true, if writing succeeded, false otherwise.*/
		bool writeGeometry (const COLLADAFW::Geometry * geometry) override;

		/** When this method is called, the writer must write the material.
		 @return The writer should return true, if writing succeeded, false otherwise.*/
		bool writeMaterial(const COLLADAFW::Material * material) override;

		/** When this method is called, the writer must write the effect.
		 @return The writer should return true, if writing succeeded, false otherwise.*/
		bool writeEffect(const COLLADAFW::Effect * effect) override;

		/** When this method is called, the writer must write the camera.
		 @return The writer should return true, if writing succeeded, false otherwise.*/
		bool writeCamera(const COLLADAFW::Camera * /*camera*/) override {
			return true;
		}

		/** When this method is called, the writer must write the image.
		 @return The writer should return true, if writing succeeded, false otherwise.*/
		bool writeImage(const COLLADAFW::Image * image) override;

		/** When this method is called, the writer must write the light.
		 @return The writer should return true, if writing succeeded, false otherwise.*/
		bool writeLight(const COLLADAFW::Light * light) override;

		/** Writes the animation.
		 @return True on succeeded, false otherwise.*/
		bool writeAnimation(const COLLADAFW::Animation * /*animation*/) override;

		/** When this method is called, the writer must write the AnimationList.
		 @return The writer should return true, if writing succeeded, false otherwise.*/
		bool writeAnimationList(const COLLADAFW::AnimationList * /*animationList*/) override;

		/** When this method is called, the writer must write the skin controller data.
		 @return The writer should return true, if writing succeeded, false otherwise.*/
		bool writeSkinControllerData(const COLLADAFW::SkinControllerData * skinControllerData) override;

		/** When this method is called, the writer must write the controller.
		 @return The writer should return true, if writing succeeded, false otherwise.*/
		bool writeController(const COLLADAFW::Controller * /*controller*/) override;

		/** When this method is called, the writer must write the formula.
		 @return The writer should return true, if writing succeeded, false otherwise.*/
		bool writeFormulas(const COLLADAFW::Formulas * /*formulas*/) override {
			return true;
		}

		/** When this method is called, the writer must write the kinematics scene.
		 @return The writer should return true, if writing succeeded, false otherwise.*/
		bool writeKinematicsScene(const COLLADAFW::KinematicsScene * /*kinematicsScene*/) override {
			return true;
		}
		//@}
};

}
}

#endif /* MINSG_LOADERCOLLADA_WRITER_H */
#endif /* MINSG_EXT_LOADERCOLLADA */
