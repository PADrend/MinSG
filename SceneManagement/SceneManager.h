/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include "MeshImportHandler.h"
#include <Util/AttributeProvider.h>
#include <Util/BidirectionalMap.h>
#include <functional>
#include <istream>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace Util {
class FileName;
class GenericAttributeMap;
}
namespace MinSG {
class AbstractBehaviour;
class BehaviourManager;
class Node;
class GroupNode;
class ListNode;
class State;
namespace SceneManagement {
class ImportContext;
struct ExporterContext;
typedef Util::GenericAttributeMap NodeDescription;

/**
 * Manages registered Nodes, loads/saves Scene-descriptions
 * from/to a file and handles scene-wide synchronization.
 *
 * [SceneManager]
 *
 */
class SceneManager : public Util::AttributeProvider {
	public:
		/**
		 * @name Main
		 */
		//@{
		SceneManager();
		~SceneManager();
		//@}

	// ---------------------------------------------------------------------------------

		/**
		 * @name Node creation and import
		 */
		//@{

	public:
		typedef uint32_t importOption_t;
		typedef std::function<bool (ImportContext & ctxt,const std::string & type, const NodeDescription & description, GroupNode * parent)> NodeImport_Fn_t;
		typedef std::function<bool (ImportContext & ctxt,const std::string & type, const NodeDescription & description, Node * parent)> StateImport_Fn_t;
		typedef std::function<bool (ImportContext & ctxt,const std::string & type, const NodeDescription & description, Node * parent)> BehaviourImport_Fn_t;
		typedef std::function<bool (ImportContext & ctxt,const std::string & type, const NodeDescription & description)> AdditionalDataImport_Fn_t;
		static const importOption_t IMPORT_OPTION_NONE = 0;
		static const importOption_t IMPORT_OPTION_REUSE_EXISTING_STATES = 1<<0;
		static const importOption_t IMPORT_OPTION_DAE_INVERT_TRANSPARENCY = 1<<2;
		static const importOption_t IMPORT_OPTION_USE_TEXTURE_REGISTRY = 1<<3;
		static const importOption_t IMPORT_OPTION_USE_MESH_REGISTRY = 1<<4;
		static const importOption_t IMPORT_OPTION_USE_MESH_HASHING_REGISTRY = 1<<5;

		void addNodeImporter(NodeImport_Fn_t);
		void addStateImporter(StateImport_Fn_t);
		void addBehaviourImporter(BehaviourImport_Fn_t);
		void addAdditionalDataImporter(AdditionalDataImport_Fn_t);
		bool processDescription(ImportContext & ctxt,const NodeDescription & d, Node * parent);
		void handleAdditionalData(ImportContext & ctxt,const NodeDescription & d);

		MeshImportHandler * getMeshImportHandler() {
			return meshImportHandler.get();
		}
		//! @note This object takes ownership of the import handler and will delete it when it is not needed anymore.
		void setMeshImportHandler(MeshImportHandler * handler) {
			meshImportHandler.reset(handler);
		}

		/**
		 * Load MinSG nodes from a file.
		 * 
		 * @param fileName Path to a MinSG XML file
		 * @param importOptions Options controlling the import procedure
		 * @return Array of MinSG nodes. In case of an error, an empty array will be returned.
		 */
		std::deque<Util::Reference<Node>> loadMinSGFile(const Util::FileName & fileName, const importOption_t importOptions = IMPORT_OPTION_NONE);

		/**
		 * Load MinSG nodes from a file.
		 * 
		 * @param importContext Context that is used for the import procedure
		 * @param fileName Path to a MinSG XML file
		 * @return Array of MinSG nodes. In case of an error, an empty array will be returned.
		 */
		std::deque<Util::Reference<Node>> loadMinSGFile(ImportContext & importContext, const Util::FileName & fileName);

		/**
		 * Load MinSG nodes from a stream.
		 * 
		 * @param importContext Context that is used for the import procedure
		 * @param in Input stream providing MinSG XML data
		 * @return Array of MinSG nodes. In case of an error, an empty array will be returned.
		 */
		std::deque<Util::Reference<Node>> loadMinSGStream(ImportContext & importContext, std::istream & in);

		GroupNode * loadCOLLADA(const Util::FileName & fileName,const importOption_t importOptions=IMPORT_OPTION_NONE);
		GroupNode * loadCOLLADA(ImportContext & importContext, const Util::FileName & fileName);

		ImportContext createImportContext(const importOption_t importOptions=IMPORT_OPTION_NONE);

	private:
		void buildSceneFromDescription(ImportContext & importContext,const NodeDescription * d);

		std::vector<NodeImport_Fn_t> nodeImporter;
		std::vector<StateImport_Fn_t> stateImporter;
		std::vector<BehaviourImport_Fn_t> behaviourImporter;
		std::vector<AdditionalDataImport_Fn_t> additionalDataImporter;

		//! The import handler for meshes is called whenever a mesh has to be created.
		std::unique_ptr<MeshImportHandler> meshImportHandler;
		//@}

	// ---------------------------------------------------------------------------------

		/**
		 * @name Node export
		 */
		//@{

	public:
		typedef std::function<void (ExporterContext & ctxt,NodeDescription&,Node * node)> NodeExport_Fn_t;
		typedef std::function<void (ExporterContext & ctxt,NodeDescription&,State * state)> StateExport_Fn_t;
		typedef std::function<NodeDescription *(ExporterContext & ctxt,AbstractBehaviour * behaviour)> BehaviourExport_Fn_t;
		void addNodeExporter(const Util::StringIdentifier & classId,NodeExport_Fn_t);
		void addStateExporter(const Util::StringIdentifier & classId,StateExport_Fn_t);
		void addBehaviourExporter(BehaviourExport_Fn_t);
		NodeDescription * createDescriptionForNode(ExporterContext & ctxt,Node * node)const;
		NodeDescription * createDescriptionForState(ExporterContext & ctxt,State * state)const;
		NodeDescription * createDescriptionForBehaviour(ExporterContext & ctxt,AbstractBehaviour * behaviour)const;
		NodeDescription * createDescriptionForScene(ExporterContext & ctxt, const std::deque<Node *> &nodes);

		/**
		 * Save MinSG nodes to a file.
		 * 
		 * @param fileName Path that the new MinSG XML file will be saved to
		 * @param nodes Array of nodes that will be saved
		 * @return @c true if successful, @c false otherwise
		 */
		bool saveMinSGFile(const Util::FileName & fileName, const std::deque<Node *> & nodes);

		/**
		 * Save MinSG nodes to a stream.
		 * 
		 * @param exportContext Context that is used for the export procedure
		 * @param out Output stream to which the MinSG XML data will be written
		 * @param nodes Array of nodes that will be saved
		 * @return @c true if successful, @c false otherwise
		 */
		bool saveMinSGStream(ExporterContext & exportContext, std::ostream & out, const std::deque<Node *> & nodes);

		/*!	Traverses the scene graph below @a rootNode and saves all meshes
			that are found in GeometryNodes and that are not saved yet into PLY
			files in a separate directory.

			@param rootNode Root of scene graph that will be traversed.
			@param dirName Name of directory that is used to store the meshes.
			@param saveRegisteredNodes If true, even already saved meshes are exported.

			\todo Shouldn't this be a StdNodeVisitor or a static helper	function in Helper.cpp? */
		void saveMeshesInSubtreeAsPLY(Node * rootNode,const std::string & dirName,
					bool saveRegisteredNodes=false)const;

		/*!	Traverses the scene graph below @a rootNode and saves all meshes
			that are found in GeometryNodes and that are not saved yet into MMF
			files in a separate directory.

			@param rootNode Root of scene graph that will be traversed.
			@param dirName Name of directory that is used to store the meshes.
			@param saveRegisteredNodes If true, even already saved meshes are exported.
			\todo Shouldn't this be a StdNodeVisitor or a static helper	function in Helper.cpp? */
		void saveMeshesInSubtreeAsMMF(Node * rootNode,const std::string & dirName,
					bool saveRegisteredNodes=false)const;

	private:
		std::unordered_map<Util::StringIdentifier,NodeExport_Fn_t> nodeExporter;
		std::unordered_map<Util::StringIdentifier,StateExport_Fn_t> stateExporter;
		std::vector<BehaviourExport_Fn_t> behaviourExporter;
		//@}

	// ---------------------------------------------------------------------------------

		/**
		 * @name Node Registration
		 */
		//@{
	public:
		/**
		 * Associate a Node with a name.
		 * The name is NOT saved inside the node.
		 * A Node can have only one name and names are unique per SceneManager.
		 *
		 * @param name The associated unique name of the node.
		 * @param node The Node to register.
		 */
		void registerNode(const Util::StringIdentifier & name, Node * node);

		/**
		 * If the Node is not already registered, register the node with a new random name.
		 *
		 * @param node The Node to register.
		 */
		void registerNode(Node * node);

		/**
		 * Traverse the given subtree and register all (not already registered) GeometryNodes.
		 *
		 * @param rootNode The root node of the subtree.
		 */
		void registerGeometryNodes(Node * rootNode);

		/**
		 * Remove the association of a Node with the given name.
		 *
		 * @param name A registered name.
		 */
		void unregisterNode(const Util::StringIdentifier & name);

		/**
		 * Return the registered name of the given Node.
		 *
		 * @param node The Node.
		 * @return The associated name or "", if @a node was not registered.
		 */
		std::string getNameOfRegisteredNode(Node * node) const;
		Util::StringIdentifier getNameIdOfRegisteredNode(Node * node) const;

		/**
		 * Return the Node associated with the given name.
		 *
		 * @param name The registered name.
		 * @return The registered Node or nullptr, if the name was not found.
		 */
		Node * getRegisteredNode(const Util::StringIdentifier & name) const;

		/**
		 * Return the names of all registered Nodes.
		 *
		 * @param names List of names that is returned.
		 */
		void getNamesOfRegisteredNodes(std::vector<std::string> & names) const;

		/**
		 * Check if the given node is registered.
		 *
		 * @param node The node.
		 * @retval @c true, if @a node is registered.
		 * @retval @c false, if @a node is not registered or @a node is @c nullptr.
		 */
		bool isNodeRegistered(Node * node) const {
			return nodeRegistry.findRight(node) != nodeRegistry.endRight();
		}

		Node * createInstance(const Util::StringIdentifier & id) const;
	private:
		typedef Util::BidirectionalMap<	std::unordered_map<Util::StringIdentifier, Util::Reference<Node> >,
											std::unordered_map<Node*, Util::StringIdentifier>,
											Util::BidirectionalMapPolicies::convertByIdentity, 	// convert_leftKeyToRightMapped_t: Util::StringIdentifier -> Util::StringIdentifier
											Util::BidirectionalMapPolicies::convertByIdentity, 	// convert_rightMappedToLeftKey_t: Util::StringIdentifier <- Util::StringIdentifier
											Util::BidirectionalMapPolicies::convertByGet,  		// convert_leftMappedToRightKey_t: Reference -> ptr*
											Util::BidirectionalMapPolicies::convertByIdentity	// convert_rightKeyToLeftMapped_t: Reference <- ptr*
						> nodeRegistry_t;

		nodeRegistry_t nodeRegistry;
		//@}

	// ---------------------------------------------------------------------------------

		/**
		 * @name Registered States
		 */
		//@{
	public:
		/*!	Associates a State with a name. The name is NOT saved inside the State. A
			State can have only one name; names are unique per SceneManager.
			@param name The associated unique name of the state.
			@param state The Node.	*/
		void registerState( const std::string & name, State * state );

		/*!	If the State is not already registered, the state is registered with a
			new random Name.
			@param state The Node.	*/
		void registerState( State * state);


		/*!	The association of a state with a given name is removed.
			@param name A registered name.	*/
		void unregisterState( const std::string & name);

		/*!	Returns the registered name of a State.
			@param state The State.
			@return The associated name or "", if state was not regiestered.	*/
		std::string getNameOfRegisteredState(State * state ) const;

		/*!	Returns the State registered to a name.
			@param name The registered name.
			@return The registered State or nullptr.	*/
		State * getRegisteredState( const std::string & name)const;
		void getNamesOfRegisteredStates(std::vector<std::string> &names)const;

	private:
		typedef Util::BidirectionalMap<	std::unordered_map<Util::StringIdentifier, Util::Reference<State> >,
											std::unordered_map<State*, Util::StringIdentifier>,
											Util::BidirectionalMapPolicies::convertByIdentity, 	// convert_leftKeyToRightMapped_t: Util::StringIdentifier -> Util::StringIdentifier
											Util::BidirectionalMapPolicies::convertByIdentity, 	// convert_rightMappedToLeftKey_t: Util::StringIdentifier <- Util::StringIdentifier
											Util::BidirectionalMapPolicies::convertByGet,  		// convert_leftMappedToRightKey_t: Reference -> ptr*
											Util::BidirectionalMapPolicies::convertByIdentity	// convert_rightKeyToLeftMapped_t: Reference <- ptr*
						> stateRegistry_t;

		stateRegistry_t stateRegistry;
		//@}

	// ---------------------------------------------------------------------------------
		/**
		 * @name Behaviour management
		 */
		//@{
	public:
		BehaviourManager * getBehaviourManager();
		const BehaviourManager * getBehaviourManager()const;
	private:
		mutable Util::Reference<BehaviourManager> behaviourManager;
		//@}
	protected:
		//void bind(std::mem_fn arg1, AbstractImporter * i, _1 arg3, _2 arg4, _3 arg5, _4 arg6);
};
}
}
#endif // SCENEMANAGER_H
