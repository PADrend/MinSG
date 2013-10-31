/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TREE_SYNC
#include "Server.h"
#include "../../Core/Nodes/Node.h"
#include "../../SceneManagement/SceneManager.h"
#include <Util/Concurrency/UserThread.h>
#include <Util/Utils.h>
#include <memory>

namespace MinSG {
namespace TreeSync {
//
		

//! (ctor)
Server::Server(SceneManagement::SceneManager & sm, Util::Network::DataBroadcaster * _broadcaster) : 
			sceneManager(sm),dataBroadcaster(_broadcaster) {
}

//! (dtor)
Server::~Server(){
	std::cout <<"~Server";
	
	// \todo ...
}
	
void Server::initNodeObserver(Node * rootNode){
	rootNode->clearTransformationObservers();
	rootNode->addTransformationObserver(std::bind(&Server::onNodeTransformed,this,std::placeholders::_1));
	std::cout << "TreeSync::Server::initNodeObserver !\n";
}

//void Server::completeRefresh(){
//}
static const uint16_t CHANNEL_MINSG_NODE_MATRIX = 0x1701;
	
void Server::onNodeTransformed(Node * node){
	const Util::StringIdentifier nodeId = sceneManager.getNameIdOfRegisteredNode(node);
	
	if(nodeId.empty())
		return;

	const Geometry::Matrix4x4 * matrix = node->getMatrixPtr();
	if(matrix == nullptr) {
		return;
	}
	const uint8_t * mData = reinterpret_cast<const uint8_t *>(matrix->getData());
	const std::vector<uint8_t> data(mData, mData + 16 * sizeof(float));

	dataBroadcaster->sendKeyValue(CHANNEL_MINSG_NODE_MATRIX,nodeId,data);
}
	
// -------------------------------------------------


//! (ctor)
TreeSyncClient::TreeSyncClient(Util::Network::DataConnection * _connection) : 
			dataConnection(_connection) {
	
	dataConnection->registerKeyValueChannelHandler(CHANNEL_MINSG_NODE_MATRIX,
						std::bind(&TreeSyncClient::_handleIncomingKeyValue, this,
										std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
	std::cout << "TreeSync::TreeSyncClient !\n";

}

//! (dtor)
TreeSyncClient::~TreeSyncClient(){
	dataConnection->removeValueChannelHandler(CHANNEL_MINSG_NODE_MATRIX);
}

void TreeSyncClient::execute(SceneManagement::SceneManager & sm){

	dataConnection->handleIncomingData(); // should be called externally!
	for(auto & idToMatrix: incomingMatrixes){
		Node * node = sm.getRegisteredNode(idToMatrix.first);
// 		std::cout <<" #" <<idToMatrix.first.toString() <<"< ";
		if(node){
			node->setMatrix(idToMatrix.second);
		}
	}
	incomingMatrixes.clear();

}


void TreeSyncClient::_handleIncomingKeyValue(uint16_t ,const Util::StringIdentifier &id,const Util::Network::DataConnection::dataPacket_t &data){
	 //(msg.first == CHANNEL_MINSG_NODE_MATRIX)
	if(data.size()!=sizeof(float)*16){
		WARN("TreeSync received invalid matrix.");
		return;
	}
	incomingMatrixes[id] = Geometry::Matrix4x4(reinterpret_cast<const float*>(data.data()));
}

}
}

#endif /* MINSG_EXT_TREE_SYNC */
