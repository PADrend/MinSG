/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT

#include "StreamFSProvider.h"

#include "ClientUnit.h"
#include "Session.h"
#include "MessageHandler.h"
#include "MessageDispatcher.h"
#include "Message.h"
#include "Tools.h"
#include "IOStreamHandler.h"
#include "Utils/StreamManipulators.h"

#include <Util/Concurrency/Concurrency.h>
#include <Util/Concurrency/Mutex.h>
#include <Util/Concurrency/Semaphore.h>
#include <Util/Concurrency/Lock.h>
#include <Util/IO/FileName.h>
#include <Util/IO/FileUtils.h>
#include <Util/Macros.h>
#include <Util/Factory/Factory.h>
#include <Util/JSON_Parser.h>
#include <Util/GenericAttribute.h>
#include <Util/StringUtils.h>
#include <Util/StringIdentifier.h>
#include <Util/Utils.h>

#include <memory>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iterator>

#define READ_BUFFER_SIZE 4096
#define NETWORK_TIMEOUT 5000

namespace D3Fact {

enum MessageType_t {
	MSGTYPE_RESOURCE_GET				    = 2010,
	MSGTYPE_RESOURCE_RESOLVE_URI		    = 2050,
	MSGTYPE_RESOURCE_URI_RESOLVED		    = 2051,
	MSGTYPE_RESOURCE_RESOLVE_URI_ERROR	    = 2052,
	MSGTYPE_RESOURCE_OPEN_STREAM 		    = 2060,
	MSGTYPE_RESOURCE_STREAM_OPENED	 	    = 2061,
	MSGTYPE_RESOURCE_OPEN_STREAM_ERROR	    = 2062,
	MSGTYPE_RESOURCE_EVENT_OBJECT_CREATED 	= 2065,
	MSGTYPE_RESOURCE_EVENT_OBJECT_DELETED 	= 2066,
	MSGTYPE_RESOURCE_DELETE_URI 		    = 2070,
	MSGTYPE_RESOURCE_URI_DELETED		    = 2071,
	MSGTYPE_RESOURCE_DELETE_URI_ERROR 		= 2072,
	MSGTYPE_RESOURCE_CREATE_DIRECTORY 		= 2080,
	MSGTYPE_RESOURCE_DIRECTORY_CREATED 		= 2081,
	MSGTYPE_RESOURCE_CREATE_DIRECTORY_ERROR	= 2082,
	MSGTYPE_RESOURCE_COPY_RESOURCES			= 2090,
	MSGTYPE_RESOURCE_RESOURCES_COPIED		= 2091,
	MSGTYPE_RESOURCE_COPY_RESOURCE_ERROR	= 2092
};

enum StreamOption_t {
	OPTION_CREATE_IN_ANY_CASE = 1,
	OPTION_CREATE_ONLY_WHEN_NEW = 2,
	OPTION_APPEND_TO_RESOURCE = 4,
	OPTION_BLOCK_CALL = 8,
	OPTION_DELETE_ON_CLOSE = 16,
	OPTION_DELETE_ON_FAIL = 32,
	OPTION_TRY_TO_OPEN = 64
};

enum ResourceType_t {
	RESOURCE_RESOLVE_TYPE_DIRECTORY = -1,
	RESOURCE_RESOLVE_TYPE_RESOURCE	= 0,
	RESOURCE_RESOLVE_TYPE_FILE		= 1
};

class StreamFSProvider::ResourceMessageHandler : public MessageHandler {
public:
	ResourceMessageHandler(StreamFSProvider* provider_) : MessageHandler(ASYNC), provider(provider_) {}
	ResourceMessageHandler(const ResourceMessageHandler &) = delete;
	ResourceMessageHandler(ResourceMessageHandler &&) = delete;
	ResourceMessageHandler & operator=(const ResourceMessageHandler &) = delete;
	ResourceMessageHandler & operator=(ResourceMessageHandler &&) = delete;
private:
	void handleMessage(Message* msg) override;
	StreamFSProvider* provider;
};

class StreamFSProvider::ResourceHandle {
public:
	enum resolved_status_t{
		UNRESOLVED, OK, PENDING
	};
	enum stream_status_t{
		STREAM_WAITING, STREAM_READY, STREAM_FAILED
	};

	explicit ResourceHandle(const std::string & uri_, int32_t clientId_, int32_t sessionId_, int32_t streamId_)
		: uri(uri_), clientId(clientId_), sessionId(sessionId_),
		  streamId(streamId_), tempStore(), dataWritten(false), resolved(UNRESOLVED), streamStatus(STREAM_READY),
		  waitResolve(Util::Concurrency::createSemaphore()), waitStream(Util::Concurrency::createSemaphore()),
		  type(0), resExists(false), mimetype(""), size(-1), timestamp(0), readable(false), writable(false), children(),
		  handler(nullptr) {}
	ResourceHandle(const ResourceHandle &) = delete;
	ResourceHandle(ResourceHandle &&) = delete;
	ResourceHandle & operator=(const ResourceHandle &) = delete;
	ResourceHandle & operator=(ResourceHandle &&) = delete;

	~ResourceHandle() {
		delete waitResolve;
		delete waitStream;
	}

	status_t readFile(std::vector<uint8_t> & data);
	status_t writeFile(const std::vector<uint8_t> & data, bool overwrite);

	status_t dir(std::list<Util::FileName> & result, const uint8_t flags);

	bool exists() {
		resolve(true);
		return resExists;
	}
	bool isFile() { return !isDir(); }
	bool isDir() {
		return uri.rfind("/") == uri.size()-1;
	}
	size_t fileSize() {
		resolve(true);
		return size;
	}

	status_t makeDir() { return UNSUPPORTED; }
	status_t removeDir() { return UNSUPPORTED; }

	resolved_status_t resolve(bool wait);

	bool isChanged() const {
		return dataWritten;
	}

	std::unique_ptr<std::istream> openForReading();
	std::ostream* openForWriting();
	std::ostream* openForAppending();

	void parseProperties(const std::string& props);

	const std::string uri;
	const int32_t clientId;
	const int32_t sessionId;
	const int32_t streamId;

	std::list<std::vector<uint8_t>> tempStore;

	bool dataWritten;
	resolved_status_t resolved;
	stream_status_t streamStatus;

	Util::Concurrency::Semaphore* waitResolve;
	Util::Concurrency::Semaphore* waitStream;

	int8_t type;
	bool resExists;
	std::string mimetype;
	int32_t size;
	int32_t timestamp;
	bool readable;
	bool writable;
	std::vector<std::string> children;

	Util::Reference<IOStreamHandler> handler;
};

StreamFSProvider::status_t StreamFSProvider::ResourceHandle::dir(std::list<Util::FileName> & result, const uint8_t /*flags*/) {
	if(resolve(true) != OK) {
		std::stringstream ss;
		ss << "Could not resolve resource: " << uri;
		WARN(ss.str());
		return StreamFSProvider::FAILURE;
	}

	for(auto child : this->children) {
		std::stringstream ss;
		ss << "d3fact://" << clientId << "/" << sessionId << "/" << streamId << "/" << uri << child;
		result.emplace_back(ss.str());
	}
	return StreamFSProvider::OK;
}

StreamFSProvider::status_t StreamFSProvider::ResourceHandle::readFile(std::vector<uint8_t> & data) {
	auto stream = openForReading();

	if(!stream || stream->bad())
		return StreamFSProvider::FAILURE;

	// read data
	if(size>0) {
		data.resize(size);
		stream->read(reinterpret_cast<char *>(data.data()), size);
		// wait for eof by reading the rest of the stream
		char buffer[READ_BUFFER_SIZE];
		while(!stream->eof()) {
			stream->read(buffer, READ_BUFFER_SIZE);
		}
	} else {
		char buffer[READ_BUFFER_SIZE];
		while(!stream->eof()) {
			size_t read = stream->read(buffer, READ_BUFFER_SIZE).gcount();
			size_t data_size = data.size();
			data.resize(data_size + read);
			char* begin = reinterpret_cast<char *>(data.data()) + data_size;
			std::copy(buffer, buffer + read, begin);
		}
	}
	return StreamFSProvider::OK;
}

StreamFSProvider::status_t StreamFSProvider::ResourceHandle::writeFile(const std::vector<uint8_t> & data, bool /*overwrite*/) {
	std::ostream* stream = openForWriting();

	if(stream == nullptr || stream->bad())
		return StreamFSProvider::FAILURE;

	stream->write(reinterpret_cast<const char *>(data.data()), data.size());
	*stream << closestream;

	handler->waitFor(streamId);

	delete stream;

	return StreamFSProvider::OK;
}

std::unique_ptr<std::istream> StreamFSProvider::ResourceHandle::openForReading() {
	streamStatus = STREAM_WAITING;
	if(resolve(true) != OK) {
		std::stringstream ss;
		ss << "Could not resolve resource: " << uri;
		WARN(ss.str());
		return nullptr;
	}

	Session* session = handler->getSession();

	if(session->isClosed()) {
		streamStatus = STREAM_FAILED;
		return nullptr;
	}

	std::unique_ptr<std::istream> stream(handler->request(streamId));

	//std::cout << "request " << uri << " (" << streamId << ")" << std::endl;

	// request stream
	Message* msg = session->createMessage(MSGTYPE_RESOURCE_OPEN_STREAM);
	std::vector<uint8_t> body = msg->accessBody();
	Tools::appendString(body, uri);
	Tools::appendIntBig(body, streamId);
	Tools::appendByte(body, 0);
	Tools::appendIntBig(body, 0);
	msg->set(body);
	session->send(msg);

	// wait for ACK
	if(!Tools::timedWait(waitStream)) {
		handler->cancelStream(streamId);
		std::stringstream ss;
		ss << "Network timeout while opening stream for reading (streamId=" << streamId ;
		ss << ", uri=" << uri << ", session=" << session->getSessionId() << ")." ;
		WARN(ss.str());
		return nullptr;
	}

	/* old resource api
	Message* msg = session->createMessage(MSGTYPE_RESOURCE_GET);
	std::vector<uint8_t> body = msg->accessBody();
	Tools::appendIntBig(body, streamId);
	Tools::appendString(body, uri);
	msg->set(body);
	session->send(msg);*/

	if(streamStatus == STREAM_FAILED) {
		handler->cancelStream(streamId);
		return nullptr;
	}

	return std::move(stream);
}

std::ostream* StreamFSProvider::ResourceHandle::openForWriting() {
	streamStatus = STREAM_WAITING;
	if(resolve(true) != OK) {
		std::stringstream ss;
		ss << "Could not resolve resource: " << uri;
		WARN(ss.str());
		return nullptr;
	}

	Session* session = handler->getSession();

	if(session->isClosed()) {
		streamStatus = STREAM_FAILED;
		return nullptr;
	}

	// request stream
	Message* msg = session->createMessage(MSGTYPE_RESOURCE_OPEN_STREAM);
	std::vector<uint8_t> body = msg->accessBody();
	Tools::appendString(body, uri);
	Tools::appendIntBig(body, streamId);
	Tools::appendByte(body, 1);
	Tools::appendIntBig(body, OPTION_CREATE_IN_ANY_CASE | OPTION_TRY_TO_OPEN);
	msg->set(body);
	session->send(msg);

	// wait for ACK
	if(!Tools::timedWait(waitStream)) {
		std::stringstream ss;
		ss << "Network timeout while opening stream for writing (streamId=" << streamId ;
		ss << ", uri=" << uri << ", session=" << session->getSessionId() << ")." ;
		WARN(ss.str());
		return nullptr;
	}

	if(streamStatus == STREAM_FAILED)
		return nullptr;

	std::ostream* stream = handler->send(streamId);

	return stream;
}

std::ostream* StreamFSProvider::ResourceHandle::openForAppending() {
	return nullptr;
}

StreamFSProvider::ResourceHandle::resolved_status_t StreamFSProvider::ResourceHandle::resolve(bool wait) {

	if(resolved == UNRESOLVED) {
		ClientUnit* client = ClientUnit::getClient(clientId);
		if(!client) {
			resolved = UNRESOLVED;
			std::stringstream ss;
			ss << "Client " << clientId << " does not exist!";
			WARN(ss.str());
			return UNRESOLVED;
		}
		Session* session = client->getSession(sessionId);
		if(!session) {
			resolved = UNRESOLVED;
			std::stringstream ss;
			ss << "Session " << sessionId << " does not exist!";
			WARN(ss.str());
			return UNRESOLVED;
		}

		resolved = PENDING;

		// send message RESOLVE_URI (uri(string), as(byte in {0,1}))
		Message* msg = session->createMessage(MSGTYPE_RESOURCE_RESOLVE_URI);
		std::vector<uint8_t> body = msg->accessBody();
		Tools::appendString(body, uri);
		int8_t as = isDir() ? RESOURCE_RESOLVE_TYPE_DIRECTORY : RESOURCE_RESOLVE_TYPE_RESOURCE;
		Tools::appendByte(body, as);
		msg->set(body);
		session->send(msg);
	}

	if(resolved == PENDING && wait) {
		if(!Tools::timedWait(waitResolve, NETWORK_TIMEOUT)) {
			std::stringstream ss;
			ss << "Network timeout while resolving resource: " << uri;
			WARN(ss.str());
			resolved = UNRESOLVED;
			return UNRESOLVED;
		}
	}

	return resolved;
}

void StreamFSProvider::ResourceMessageHandler::handleMessage(Message* msg) {
	if(msg->getType() == MSGTYPE_RESOURCE_URI_RESOLVED) {
		uint32_t uriLen = Tools::getInt(msg->getBody(), 0);
		std::string uri = Tools::getString(msg->getBody(), 0);
		int8_t type = Tools::getByte(msg->getBody(), uriLen+4);
		std::string properties = Tools::getString(msg->getBody(), uriLen+5);
		Util::StringIdentifier uriId(uri);

		{
			//std::cout << "resolved " << uri << std::endl;
			auto lock = Util::Concurrency::createLock(*provider->handlesMutex);
			auto range = provider->openHandles.equal_range(uriId.getValue());
			for(auto it = range.first; it != range.second; ++it) {
				if(it->second->sessionId == msg->getSession()) {
					it->second->type = type;
					it->second->parseProperties(properties);
					if(it->second->resolved == ResourceHandle::PENDING)
						it->second->resolved = ResourceHandle::OK;
					it->second->waitResolve->post();
				}
			}
		}

	} else if(msg->getType() == MSGTYPE_RESOURCE_RESOLVE_URI_ERROR) {
		std::string uri = Tools::getString(msg->getBody(), 0);
		std::string err = Tools::getString(msg->getBody(), Tools::getInt(msg->getBody(), 0) + 4);
		Util::StringIdentifier uriId(uri);

		{
			auto lock = Util::Concurrency::createLock(*provider->handlesMutex);
			auto range = provider->openHandles.equal_range(uriId.getValue());
			for(auto it = range.first; it != range.second; ++it) {
				if(it->second->sessionId == msg->getSession()) {
					it->second->resolved = ResourceHandle::UNRESOLVED;
					it->second->waitResolve->post();
				}
			}
		}
	} else if(msg->getType() == MSGTYPE_RESOURCE_STREAM_OPENED) {
		int32_t streamId = Tools::getInt(msg->getBody(), 0);

		{
			//std::cout << "stream opened (" << streamId << ")" << std::endl;
			auto lock = Util::Concurrency::createLock(*provider->handlesMutex);
			for(auto it : provider->openHandles) {
				if(it.second->sessionId == msg->getSession() && it.second->streamId == streamId) {
					it.second->streamStatus = ResourceHandle::STREAM_READY;
					it.second->waitStream->post();
				}
			}
		}
	} else if(msg->getType() == MSGTYPE_RESOURCE_OPEN_STREAM_ERROR) {
		int32_t streamId = Tools::getInt(msg->getBody(), 0);
		std::string err = Tools::getString(msg->getBody(), 4);
		WARN(err);

		{
			auto lock = Util::Concurrency::createLock(*provider->handlesMutex);
			for(auto it : provider->openHandles) {
				if(it.second->sessionId == msg->getSession() && it.second->streamId == streamId) {
					it.second->handler->cancelStream(streamId);
					it.second->streamStatus = ResourceHandle::STREAM_FAILED;
					it.second->waitStream->post();
				}
			}
		}
	}

	msg->dispose();
}

void StreamFSProvider::ResourceHandle::parseProperties(const std::string& props) {
	static const Util::StringIdentifier TYPE("type");
	static const Util::StringIdentifier RESTYPE("resourcetype");
	static const Util::StringIdentifier CHILDREN("children");
	static const Util::StringIdentifier EXISTS("exists");
	static const Util::StringIdentifier MIMETYPE("mimetype");
	static const Util::StringIdentifier SIZE("size");
	static const Util::StringIdentifier TIMESTAMP("timestamp");
	static const Util::StringIdentifier READABLE("isReadable");
	static const Util::StringIdentifier WRITABLE("isWritable");

	Util::GenericAttributeMap* attr = dynamic_cast<Util::GenericAttributeMap*>(Util::JSON_Parser::parse(props));

	if(!attr)
		return;

	if(type == RESOURCE_RESOLVE_TYPE_RESOURCE || type == RESOURCE_RESOLVE_TYPE_FILE) {
		type = attr->getInt(TYPE,0);
		resExists = attr->getBool(EXISTS, false);
		mimetype = attr->getString(MIMETYPE, "");
		size = attr->getInt(SIZE, -1);
		timestamp = attr->getInt(TIMESTAMP, 0);
		readable = attr->getBool(READABLE, false);
		writable = attr->getBool(WRITABLE, false);
	} else {
		type = attr->getInt(RESTYPE,0);
		resExists = attr->getBool(EXISTS, false);
		writable = attr->getBool(WRITABLE, false);
		Util::GenericAttributeList* childList = dynamic_cast<Util::GenericAttributeList*>(attr->getValue(CHILDREN));
		if(childList) {
			Util::GenericAttributeList::iterator it;
			for(it = childList->begin(); it != childList->end(); ++it) {
				children.push_back((*it)->toString());
			}
		}
	}
}

bool StreamFSProvider::init() {
	static StreamFSProvider provider;
	return Util::FileUtils::registerFSProvider("d3fact", Util::PointerHolderCreator<StreamFSProvider>(&provider));
}

StreamFSProvider::StreamFSProvider() :
		AbstractFSProvider(), openHandles(), handlesMutex(Util::Concurrency::createMutex()), msgHandler(new ResourceMessageHandler(this)) {
}

StreamFSProvider::~StreamFSProvider() {
	delete handlesMutex;
}

bool StreamFSProvider::exists(const Util::FileName& filename) {
	ResourceHandle* handle = getStreamHandle(filename);
	return handle ? handle->exists() : false;
}

bool StreamFSProvider::isFile(const Util::FileName& filename) {
	ResourceHandle* handle = getStreamHandle(filename);
	return handle ? handle->isFile() : false;
}

bool StreamFSProvider::isDir(const Util::FileName& filename) {
	ResourceHandle* handle = getStreamHandle(filename);
	return handle ? handle->isDir() : false;
}

size_t StreamFSProvider::fileSize(const Util::FileName& filename) {
	ResourceHandle* handle = getStreamHandle(filename);
	return handle ? handle->fileSize() : 0;
}

StreamFSProvider::status_t StreamFSProvider::makeDir(const Util::FileName& /*filename*/) {
	return UNSUPPORTED;
}

StreamFSProvider::status_t StreamFSProvider::remove(const Util::FileName& /*filename*/) {
	return UNSUPPORTED;
}

StreamFSProvider::status_t StreamFSProvider::dir(const Util::FileName& path,
		std::list<Util::FileName>& result, uint8_t flags) {
	ResourceHandle* handle = getStreamHandle(path);
	return handle ? handle->dir(result, flags) : FAILURE;
}

StreamFSProvider::status_t StreamFSProvider::readFile(const Util::FileName& filename,
		std::vector<uint8_t>& data) {
	ResourceHandle* handle = getStreamHandle(filename);
	return handle ? handle->readFile(data) : FAILURE;
}

StreamFSProvider::status_t StreamFSProvider::writeFile(const Util::FileName& filename,
		const std::vector<uint8_t>& data, bool overwrite) {
	ResourceHandle* handle = getStreamHandle(filename);
	return handle ? handle->writeFile(data, overwrite) : FAILURE;
}

std::iostream* StreamFSProvider::open(const Util::FileName& /*filename*/) {
	return nullptr;
}

std::unique_ptr<std::istream> StreamFSProvider::openForReading(const Util::FileName& filename) {
	ResourceHandle* handle = getStreamHandle(filename);
	return handle ? handle->openForReading() : nullptr;
}

std::ostream* StreamFSProvider::openForWriting(const Util::FileName& filename) {
	ResourceHandle* handle = getStreamHandle(filename);
	return handle ? handle->openForWriting() : nullptr;
}

std::ostream* StreamFSProvider::openForAppending(const Util::FileName& filename) {
	ResourceHandle* handle = getStreamHandle(filename);
	return handle ? handle->openForAppending() : nullptr;
}

StreamFSProvider::ResourceHandle* StreamFSProvider::getStreamHandle(const Util::FileName& filename) {

	ResourceHandle* handle = nullptr;

	int32_t clientId;
	int32_t sessionId;
	int32_t streamId;
	std::string resourcePath;

	decomposeURL(filename, clientId, sessionId, streamId, resourcePath);

	Util::StringIdentifier uriId(resourcePath);

	{
		auto lock = Util::Concurrency::createLock(*handlesMutex);
		auto range = openHandles.equal_range(uriId.getValue());
		for(auto it = range.first; it != range.second; ++it) {
			if(it->second->clientId == clientId && it->second->sessionId == sessionId && it->second->streamId == streamId)
				return it->second;
		}
	}

	ClientUnit* client = ClientUnit::getClient(clientId);
	if(!client)
		return nullptr;

	Session* session = client->getSession(sessionId);
	if(!session)
		return nullptr;

	if(!session->getDispatcher()->hasHandler(MSGTYPE_RESOURCE_URI_RESOLVED)) {
		session->getDispatcher()->registerHandler(msgHandler.get(), MSGTYPE_RESOURCE_URI_RESOLVED);
		session->getDispatcher()->registerHandler(msgHandler.get(), MSGTYPE_RESOURCE_RESOLVE_URI_ERROR);
		session->getDispatcher()->registerHandler(msgHandler.get(), MSGTYPE_RESOURCE_STREAM_OPENED);
		session->getDispatcher()->registerHandler(msgHandler.get(), MSGTYPE_RESOURCE_OPEN_STREAM_ERROR);
	}

	IOStreamHandler* handler = nullptr;
	if(!session->getDispatcher()->hasHandler(STREAM_HEADER)) {
		handler = new IOStreamHandler(session);
		session->getDispatcher()->registerHandler(handler, STREAM_HEADER);
		session->getDispatcher()->registerHandler(handler, STREAM_INTERMEDIATE);
		session->getDispatcher()->registerHandler(handler, STREAM_TAIL);
		session->getDispatcher()->registerHandler(handler, STREAM_ERROR);
	} else {
		handler = dynamic_cast<IOStreamHandler*>(session->getDispatcher()->getHandler(STREAM_HEADER));
		if(handler == nullptr) {
			WARN("Could not register StreamHandler");
			return nullptr;
		}
	}

	handle = new ResourceHandle(resourcePath, clientId, sessionId, streamId);
	handle->handler = handler;
	//handle->resolve(false);

	{
		auto lock = Util::Concurrency::createLock(*handlesMutex);
		openHandles.insert(std::make_pair(uriId.getValue(), handle));
	}

	return handle;
}

void StreamFSProvider::flush() {
	auto lock = Util::Concurrency::createLock(*handlesMutex);
	for(auto & openHandle : openHandles) {
		delete openHandle.second;
	}
	openHandles.clear();
}

void StreamFSProvider::decomposeURL(const Util::FileName& url,
		int32_t& clientId, int32_t& sessionId, int32_t & streamId, std::string& resourcePath) {

	// d3fact uris are composed of "d3fact://clientId/sessionId/streamId/scheme:/path/to/resource"

	// get uri without "d3fact://"
	std::string uri = url.getPath();

	// parse clientId
	size_t pos1 = 0;
	size_t pos2 = uri.find("/");
	std::string client = uri.substr(pos1, pos2-pos1);

	// parse sessionId
	pos1 = pos2+1;
	pos2 = uri.find("/", pos1);
	std::string session = uri.substr(pos1, pos2-pos1);

	// parse streamId
	pos1 = pos2+1;
	pos2 = uri.find("/", pos1);
	std::string stream = uri.substr(pos1, pos2-pos1);

	clientId = Util::StringUtils::toNumber<int32_t>(client);
	sessionId = Util::StringUtils::toNumber<int32_t>(session);
	streamId = Util::StringUtils::toNumber<int32_t>(stream);

	// the actual uri to the resource is the remaining path after streamId
	resourcePath = uri.substr(pos2+1);

	if(Util::StringUtils::beginsWith(resourcePath.c_str(), "mesh") &&
			Util::StringUtils::beginsWith(resourcePath.substr(resourcePath.length()-4).c_str(), ".mmf")) {
		resourcePath = resourcePath.substr(0, resourcePath.length()-4);
	} else if(Util::StringUtils::beginsWith(resourcePath.c_str(), "texture") &&
			Util::StringUtils::beginsWith(resourcePath.substr(resourcePath.length()-4).c_str(), ".png")) {
		resourcePath = resourcePath.substr(0, resourcePath.length()-4);
	}
}


} /* namespace D3Fact */
#endif /* MINSG_EXT_D3FACT */
