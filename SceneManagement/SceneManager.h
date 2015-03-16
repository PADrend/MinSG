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

#include <Util/AttributeProvider.h>
#include <iosfwd>
#include <string>
#include <vector>


namespace MinSG {
class BehaviourManager;
class Node;
class State;
namespace SceneManagement {


template<class T> class TreeRegistry;

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
		 * @name Node Registration
		 */
		//@{
	private:
		std::unique_ptr<TreeRegistry<Node>> nodeRegistry;
	
	public:
		/**
		 * Associate a Node with an id.
		 * The id is NOT saved inside the node.
		 * A Node can have only one id and ids are unique per SceneManager.
		 *
		 * @param id The associated unique id of the node.
		 * @param node The Node to register.
		 */
		void registerNode(const Util::StringIdentifier & id, Util::Reference<Node> node);

		//! If the @p Node is not registered, register the node with a new random id.
		void registerNode(Util::Reference<Node> node);

		//!	Traverse the given @p subtree and register all GeometryNodes which are not registered.
		void registerGeometryNodes(Node * rootNode);

		//!	Remove the association of a Node with the given @p id.
		void unregisterNode(const Util::StringIdentifier & id);

		//!	Return the registered id of the given @p Node. The result is empty if the node is not registered. 
		std::string getNameOfRegisteredNode(Node * node) const				{	return getNodeId(node).toString();	} // deptrecated
		Util::StringIdentifier getNameIdOfRegisteredNode(Node * node)const	{	return getNodeId(node); }; // deprecated
		Util::StringIdentifier getNodeId(Node * node) const;

		//!	Return the Node associated with the given @p id or nullptr, if ther is no node with that id.
		Node * getRegisteredNode(const Util::StringIdentifier & id) const;

		/**
		 * Return the ids of all registered Nodes.
		 *
		 * @param ids List of ids that is returned.
		 */
		void getNamesOfRegisteredNodes(std::vector<std::string> & ids) const; // deprecated
		std::vector<Util::StringIdentifier> getNodeIds()const;

		/**
		 * Check if the given node is registered.
		 *
		 * @param node The node.
		 * @retval @c true, if @a node is registered.
		 * @retval @c false, if @a node is not registered or @a node is @c nullptr.
		 */
		bool isNodeRegistered(Node * node) const; 

		Node * createInstance(const Util::StringIdentifier & id) const;

		//@}

	// ---------------------------------------------------------------------------------

		/**
		 * @name Registered States
		 */
		//@{
	private:
		std::unique_ptr<TreeRegistry<State>> stateRegistry;
	public:
		/*!	Associates a State with a id. The id is NOT saved inside the State. A
			State can have only one id; ids are unique per SceneManager.
			@param id The associated unique id of the state.
			@param state The Node.	*/
		void registerState(const Util::StringIdentifier & id, Util::Reference<State> state );

		/*!	If the State is not already registered, the state is registered with a
			new random Name.
			@param state The Node.	*/
		void registerState( Util::Reference<State> state);


		/*!	The association of a state with a given id is removed.
			@param id A registered id.	*/
		void unregisterState(const Util::StringIdentifier & id);

		/*!	Returns the registered id of a State.
			@param state The State.
			@return The associated id or "", if state was not regiestered.	*/
		std::string getNameOfRegisteredState(State * state ) const		{	return getStateId(state).toString();	} // deprecated
		Util::StringIdentifier getStateId(State * state) const;

		/*!	Returns the State registered to a id.
			@param id The registered id.
			@return The registered State or nullptr.	*/
		State * getRegisteredState(const Util::StringIdentifier& id)const;
		
		std::vector<Util::StringIdentifier> getStateIds()const;
		void getNamesOfRegisteredStates(std::vector<std::string> &ids)const; // deprecated

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
};
}
}
#endif // SCENEMANAGER_H
