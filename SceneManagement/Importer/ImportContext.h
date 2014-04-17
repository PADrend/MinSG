/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_IMPORT_CONTEXT_H
#define MINSG_IMPORT_CONTEXT_H

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Texture/Texture.h>
#include <Util/AttributeProvider.h>
#include <Util/References.h>
#include <Util/IO/FileName.h>
#include <Util/IO/FileLocator.h>
#include <functional>
#include <deque>
#include <map>
#include <cstdint>
#include <string>
#include <vector>

namespace MinSG {
class GroupNode;
namespace SceneManagement {
class SceneManager;

/*! Helper structure that keeps data for one import process. */
class ImportContext : public Util::AttributeProvider {
	public:
		SceneManager & sceneManager;
		GroupNode * rootNode;
		uint32_t importOptions;
		Util::FileName fileName;
		Util::FileLocator fileLocator;

		//! A function that allows the execution of arbitrary actions at the end of the import process.
		typedef std::function<void (ImportContext &)> FinalizeAction;
		std::deque<FinalizeAction> finalizeActions;

		/*! (ctor) */
		ImportContext(SceneManager & _m,GroupNode * _rootNode,uint32_t _f,Util::FileName path) :
				Util::AttributeProvider(), sceneManager(_m),rootNode(_rootNode),importOptions(_f),fileName(std::move(path)),finalizeActions(){
		}

		void addFinalizingAction(const FinalizeAction & action) {	finalizeActions.push_back(action);	}
		void addSearchPath(std::string p) 					{	fileLocator.addSearchPath( std::move(p) );	}

		void executeFinalizingActions();
		uint32_t getImportOptions()const					{	return importOptions;	}
		const Util::FileName & getFileName()const			{	return fileName;	}
		GroupNode * getRootNode()const						{	return rootNode;	}

		void setFileName(const Util::FileName & f)			{	fileName = f;	}
		void setRootNode(GroupNode * n)						{	rootNode = n;	}



		/**
		 * @name Registered Textures
		 */
		//@{
	public:
		typedef std::map<const std::string, Util::Reference<Rendering::Texture> > textureRegistry_t;
		textureRegistry_t & getTextureRegistry()			{	return registeredTextures;	}

//		/*!	Associates a State with a name.  */
//		void registerTexture( const std::string & fileName, Rendering::Texture * t);
//
//		/*!	The association of a state with a given name is removed.
//			@param name A registered name.	*/
//		void unregisterTexture( const std::string & fileName);
//
//		/*!	Returns the Texture registered to a name.
//			@param name The registered name.
//			@return The registered Texture or nullptr.	*/
//		Rendering::Texture * getRegisteredTexture( const std::string & fileName)const;

	private:
		textureRegistry_t registeredTextures;
		//@}

		/**
		 * @name Registered Meshes (with filenames)
		 */
		//@{
	public:
		typedef std::map<const std::string, Util::Reference<Rendering::Mesh> > meshRegistry_t;
		meshRegistry_t & getMeshRegistry()					{	return registeredMeshes;	}
//
		/*!	Associates a Mesh with a name.  */
		void registerMesh( const std::string & fileName, Rendering::Mesh * t);

		/*!	The association of a Mesh with a given name is removed.
			@param name A registered name.	*/
		void unregisterMesh( const std::string & fileName);

		/*!	Returns the Mesh registered to a name.
			@param name The registered name.
			@return The registered Mesh or nullptr.	*/
		Rendering::Mesh * getRegisteredMesh( const std::string & fileName)const;

	private:
		meshRegistry_t registeredMeshes;
		//@}

		/**
		 * @name Registered Meshes (with hashing)
		 */
		//@{
	public:
		typedef std::vector<Util::Reference<Rendering::Mesh> > meshHashingRegistryBucket_t;
		typedef std::map<uint32_t, meshHashingRegistryBucket_t > meshHasingRegistry_t;

		/*!	Associates a Mesh with a hash value.  */
		void registerMesh( const uint32_t hash , Rendering::Mesh * m)	{	registeredHashedMeshes[hash].push_back(m); }


		/*!	Returns a Mesh with the same data as the given mesh and hash-value
			@param hash The hash of the mesh.
			@param mesh The Mesh for comparison.
			@return The registered Mesh or nullptr.	*/
		Rendering::Mesh * getRegisteredMesh(const uint32_t hash , Rendering::Mesh * m)const;

	private:
		meshHasingRegistry_t registeredHashedMeshes;
		//@}


};


}
}
#endif // MINSG_IMPORT_CONTEXT_H
