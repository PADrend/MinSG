/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT

#ifndef OBJECTREGISTRY_H_
#define OBJECTREGISTRY_H_

#include <Util/References.h>
#include <Util/StringUtils.h>
#include <Util/StringIdentifier.h>
#include <Util/Macros.h>
#include <list>

namespace D3Fact {

template<class _T>
class ObjectRegistry {
public:
	ObjectRegistry();
	virtual ~ObjectRegistry();

	void registerObject(const std::string & name, _T * obj );
	void registerObject(const Util::StringIdentifier & id, _T * obj );
	void registerObject( _T * obj );

	void unregisterObject( const std::string & name );
	void unregisterObject( const Util::StringIdentifier &  id );

	_T * getRegisteredObject(const std::string & name) const;
	_T * getRegisteredObject(const Util::StringIdentifier & id) const;

	Util::StringIdentifier getIdOfRegisteredObject( _T * obj ) const;
	void getIdsOfRegisteredObjects(std::list<Util::StringIdentifier> & ids) const;

	std::string getNameOfRegisteredObject( _T * obj ) const;
	void getNamesOfRegisteredObjects(std::list<std::string> & ids) const;

	void getRegisteredObjects(std::list<Util::Reference<_T> > & objects) const;

	void clear();

private:
	typedef std::map<Util::StringIdentifier, Util::Reference<_T> > objMap_t;
	typedef std::map<_T *, std::pair<Util::StringIdentifier, std::string> > objNameMap_t;
	typedef typename objMap_t::iterator objMap_iterator;
	typedef typename objNameMap_t::iterator objNameMap_iterator;
	typedef typename objMap_t::const_iterator objMap_const_iterator;
	typedef typename objNameMap_t::const_iterator objNameMap_const_iterator;

	objMap_t registeredObjects;
	objNameMap_t registeredObjectNames;
};


template<class _T>
ObjectRegistry<_T>::ObjectRegistry() {
}

template<class _T>
ObjectRegistry<_T>::~ObjectRegistry() {
	clear();
}

template<class _T>
void ObjectRegistry<_T>::registerObject(const std::string & name, _T * obj) {
	const Util::StringIdentifier id(name);
	if (obj == nullptr) {
		registeredObjects.erase(id);
		registeredObjectNames.erase(obj);
		return;
	}
	// Store a temporary reference to the new Object.
	Util::Reference<_T> ref = obj;
	{
		// Check if the same Object is already registered.
		objNameMap_iterator oldName = registeredObjectNames.find(ref.get());
		if (oldName != registeredObjectNames.end()) {
			if (oldName->second.second == name) {
				// The requested registration already exists.
				return;
			} else {
				// Delete both old entries.
				const Util::StringIdentifier oldId = Util::StringIdentifier(oldName->second.second);
				registeredObjects.erase(oldId);
				registeredObjectNames.erase(oldName);
			}
		}
	}
	{
		// Check if a Object with the same name is already registered.
		const Util::StringIdentifier oldId = Util::StringIdentifier(name);
		objMap_iterator oldObj = registeredObjects.find(oldId);
		if (oldObj != registeredObjects.end()) {
			// Delete both old entries.
			objNameMap_iterator oldName = registeredObjectNames.find(oldObj->second.get());
			registeredObjects.erase(oldObj);
			registeredObjectNames.erase(oldName);
		}
	}

	registeredObjects.insert(std::make_pair(id, ref));
	registeredObjectNames.insert(std::make_pair(ref.get(), std::make_pair(id, name)));
}

template<class _T>
void ObjectRegistry<_T>::registerObject(const Util::StringIdentifier & id, _T * obj) {
//  TODO an unsigned int can never be -1
// 	if(id==-1) {
// 		WARN("Could not register object with id -1! This id is reserved.");
// 		return;
// 	}
	if (obj == nullptr) {
		registeredObjects.erase(id);
		registeredObjectNames.erase(obj);
		return;
	}
	const std::string name = id.toString();
	// Store a temporary reference to the new Object.
	Util::Reference<_T> ref = obj;
	{
		// Check if the same Object is already registered.
		objNameMap_iterator oldName = registeredObjectNames.find(ref.get());
		if (oldName != registeredObjectNames.end()) {
			if (oldName->second.first == id) {
				// The requested registration already exists.
				return;
			} else {
				// Delete both old entries.
				const Util::StringIdentifier oldId = oldName->second.first;
				registeredObjects.erase(oldId);
				registeredObjectNames.erase(oldName);
			}
		}
	}
	{
		// Check if a Object with the same id is already registered.
		objMap_iterator oldObj = registeredObjects.find(id);
		if (oldObj != registeredObjects.end()) {
			// Delete both old entries.
			objNameMap_iterator oldName = registeredObjectNames.find(oldObj->second.get());
			registeredObjects.erase(oldObj);
			registeredObjectNames.erase(oldName);
		}
	}

	registeredObjects.insert(std::make_pair(id, ref));
	registeredObjectNames.insert(std::make_pair(ref.get(), std::make_pair(id, name)));
}

template<class _T>
void ObjectRegistry<_T>::registerObject(_T * obj) {
	if (obj == nullptr || !getNameOfRegisteredObject(obj).empty()) { // no node or already registered?
		return;
	}
	std::string newId;
	do {
		// Create a new, random identifier.
		newId = "$" + Util::StringUtils::createRandomString(6);
		// Make sure we get a unique identifier.
	} while (getRegisteredObject(newId) != nullptr);
	registerObject(newId, obj);
}

template<class _T>
void ObjectRegistry<_T>::unregisterObject(const std::string & name) {
	registerObject(name, nullptr);
}

template<class _T>
void ObjectRegistry<_T>::unregisterObject(const Util::StringIdentifier & id) {
	registerObject(id, nullptr);
}

template<class _T>
_T * ObjectRegistry<_T>::getRegisteredObject(const std::string & name) const {
	const Util::StringIdentifier id = Util::StringIdentifier(name);
	objMap_const_iterator it = registeredObjects.find(id);
	if (it == registeredObjects.end()) {
		return nullptr;
	}
	return it->second.get();
}

template<class _T>
_T * ObjectRegistry<_T>::getRegisteredObject(const Util::StringIdentifier & id) const {
	objMap_const_iterator it = registeredObjects.find(id);
	if (it == registeredObjects.end()) {
		return nullptr;
	}
	return it->second.get();
}

template<class _T>
void ObjectRegistry<_T>::getIdsOfRegisteredObjects(std::list<Util::StringIdentifier> & ids) const {
	for (objNameMap_const_iterator it = registeredObjectNames.begin(); it != registeredObjectNames.end(); ++it) {
		ids.push_back(it->second.first);
	}
}

template<class _T>
Util::StringIdentifier ObjectRegistry<_T>::getIdOfRegisteredObject(_T * obj) const {
	if (obj == nullptr) {
		return Util::StringIdentifier(-1);
	}
	objNameMap_const_iterator find = registeredObjectNames.find(obj);
	if (find == registeredObjectNames.end()) {
		return Util::StringIdentifier(-1);
	}
	return find->second.first;
}

template<class _T>
void ObjectRegistry<_T>::getNamesOfRegisteredObjects(std::list<std::string> & names) const {
	for (objNameMap_const_iterator it = registeredObjectNames.begin(); it != registeredObjectNames.end(); ++it) {
		names.push_back(it->second.second);
	}
}

template<class _T>
std::string ObjectRegistry<_T>::getNameOfRegisteredObject(_T * obj) const {
	if (obj == nullptr) {
		return std::string();
	}
	objNameMap_const_iterator find = registeredObjectNames.find(obj);
	if (find == registeredObjectNames.end()) {
		return std::string();
	}
	return find->second.second;
}

template<class _T>
void ObjectRegistry<_T>::getRegisteredObjects(std::list<Util::Reference<_T> > & objects) const {
	for (objMap_const_iterator it = registeredObjects.begin(); it != registeredObjects.end(); ++it) {
		objects.push_back(it->second);
	}
}

template<class _T>
void ObjectRegistry<_T>::clear() {
	registeredObjects.clear();
	registeredObjectNames.clear();
}

}
#endif /* OBJECTREGISTRY_H_ */
#endif /* MINSG_EXT_D3FACT */
