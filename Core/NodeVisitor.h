/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_NODEVISITOR_H
#define MINSG_NODEVISITOR_H

namespace MinSG {

class Node;

/***
 **  NodeVisitor
 **/
class NodeVisitor {
	public:
		enum status {
			CONTINUE_TRAVERSAL = 0,
			BREAK_TRAVERSAL = 1,
			EXIT_TRAVERSAL = 2
		};
		NodeVisitor(){
		}
		virtual ~NodeVisitor() {
		}
		/**
		 * Called when entering a Node during traversal.
		 * @return CONTINUE_TRAVERSAL   traverse the children before leaving the node
		 *         BREAK_TRAVERSAL      skip the children and leave the node
		 *         EXIT_TRAVERSAL       stop the traversal immediately (without leaving a node)
		 */
		virtual status enter(Node *) {
			return CONTINUE_TRAVERSAL;
		}
		/**
		 * Called when leaving a Node during traversal.
		 * @return CONTINUE_TRAVERSAL   continue the traversal
		 *         (BREAK_TRAVERSAL)    undefined
		 *         EXIT_TRAVERSAL       stop the traversal immediately
		 */
		virtual status leave(Node *) {
			return CONTINUE_TRAVERSAL;
		}
};

}
#endif // MINSG_NODEVISITOR_H
