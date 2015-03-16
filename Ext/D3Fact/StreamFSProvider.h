/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT

#ifndef STREAMFSPROVIDER_H_
#define STREAMFSPROVIDER_H_

#include <Util/IO/AbstractFSProvider.h>
#include <Util/IO/FileName.h>
#include <Util/References.h>

#include <iosfwd>
#include <mutex>
#include <unordered_map>

namespace D3Fact {

/**
 * File system provider for access to d3fact resources using the
 * "d3fact://clientId/sessionId/streamId/scheme/path/to/resource" URL scheme.
 *
 * @author Sascha Brandt
 * @date 2012-11-20
 */
class StreamFSProvider: public Util::AbstractFSProvider {
public:
	static bool init();

	StreamFSProvider();
	virtual ~StreamFSProvider();

	virtual bool exists(const Util::FileName & filename) override;
	virtual bool isFile(const Util::FileName & filename) override;
	virtual bool isDir(const Util::FileName & filename) override;
	virtual size_t fileSize(const Util::FileName & filename) override;

	virtual status_t makeDir(const Util::FileName & filename) override;

	virtual status_t remove(const Util::FileName & filename) override;

	virtual status_t dir(const Util::FileName & path, std::list<Util::FileName> & result, uint8_t flags) override;

	virtual status_t readFile(const Util::FileName & filename, std::vector<uint8_t> & data) override;
	virtual status_t writeFile(const Util::FileName & filename, const std::vector<uint8_t> & data, bool overwrite) override;

	virtual std::unique_ptr<std::iostream> open(const Util::FileName & filename) override;
	virtual std::unique_ptr<std::istream> openForReading(const Util::FileName & filename) override;
	virtual std::unique_ptr<std::ostream> openForWriting(const Util::FileName & filename) override;
	virtual std::unique_ptr<std::ostream> openForAppending(const Util::FileName & filename) override;

	virtual void flush() override;
private:
	StreamFSProvider(const StreamFSProvider &) = delete;
	StreamFSProvider(StreamFSProvider &&) = delete;
	StreamFSProvider & operator=(const StreamFSProvider &) = delete;
	StreamFSProvider & operator=(StreamFSProvider &&) = delete;

	friend class ResourceHandle;
	friend class ResourceMessageHandler;
	class ResourceHandle;
	class ResourceMessageHandler;

	std::unordered_multimap<uint32_t, ResourceHandle *> openHandles;
	std::mutex handlesMutex;

	Util::Reference<ResourceMessageHandler> msgHandler;

	ResourceHandle * getStreamHandle(const Util::FileName & filename);
	void decomposeURL(const Util::FileName & url, int32_t & clientId, int32_t & sessionId, int32_t & streamId, std::string & resourcePath);
};

} /* namespace D3Fact */
#endif /* STREAMFSPROVIDER_H_ */
#endif /* MINSG_EXT_D3FACT */
