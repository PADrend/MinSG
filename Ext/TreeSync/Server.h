/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius J�hn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TREE_SYNC

#ifndef TREESYNCSERVER_H_
#define TREESYNCSERVER_H_

#include <memory>
#include <set>
#include <cstdint>
#include <Util/References.h>
#include <Util/StringIdentifier.h>
#include <Util/Network/DataConnection.h>
#include <Util/Network/DataBroadcaster.h>
#include <Geometry/Matrix4x4.h>

namespace Util {
namespace Network {
class DataBroadcaster;
}
}

namespace MinSG {
class Node;
	
namespace SceneManagement {
class SceneManager;
}

//! @ingroup ext
namespace TreeSync {


// \todo make sure that the registered handles' objects are not accidentally deleted!

class Server{
		SceneManagement::SceneManager & sceneManager;
		Util::Reference<Util::Network::DataBroadcaster> dataBroadcaster;
	public:
		MINSGAPI Server(SceneManagement::SceneManager & sm, Util::Network::DataBroadcaster * _broadcaster);
		Server(const Server&) = delete;
		MINSGAPI ~Server();
	
		
		MINSGAPI void initNodeObserver(Node * rootNode);
		MINSGAPI void onNodeTransformed(Node * node);
		
};

class TreeSyncClient{
		Util::Reference<Util::Network::DataConnection> dataConnection;
		
		std::unordered_map<Util::StringIdentifier,Geometry::Matrix4x4> incomingMatrixes;
		
	public:
		MINSGAPI TreeSyncClient(Util::Network::DataConnection * _connection);
		MINSGAPI ~TreeSyncClient();
	
		/*! Applies the received transformation updates.
			Should be called once per frame.	*/
		MINSGAPI void execute(SceneManagement::SceneManager & sm);

		MINSGAPI void _handleIncomingKeyValue(uint16_t channel,const Util::StringIdentifier &,const Util::Network::DataConnection::dataPacket_t &);
		
};

}
}

#endif /* TREESYNCSERVER_H_ */

#endif /* MINSG_EXT_TREE_SYNC */
