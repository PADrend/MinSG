/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT

#include <Util/Utils.h>
#include <Util/Macros.h>
#include <Util/Network/NetworkTCP.h>
#include <Util/Network/NetworkUDP.h>
#include <Util/Concurrency/Concurrency.h>
#include <Util/Concurrency/Semaphore.h>
#include <Util/Concurrency/Mutex.h>
#include <Util/Concurrency/Lock.h>
#include <Util/Concurrency/UserThread.h>
#include <Util/Utils.h>
#include <Util/Macros.h>
#include <Util/IO/FileUtils.h>
#include <Util/IO/FileName.h>

#include "Message.h"
#include "Tools.h"

#include <random>
#include <ctime>
#include <zlib.h>
#include <tuple>
#include <deque>

using namespace Util;

namespace D3Fact {

const uint32_t Tools::TIMEOUT = 2000;

// bad workaround for semaphores with timeout
class SemTimeout : public Util::Concurrency::UserThread {
public:
	SemTimeout(Util::Concurrency::Semaphore* sem_, uint32_t ms) : Util::Concurrency::UserThread(),
	sem(sem_), mutex(Util::Concurrency::createMutex()), time(ms), timeout(false), aborted(false) {start();}

	~SemTimeout() {
		delete mutex;
	}

	bool isTimeout() { return timeout; }
	void abort() {
		auto lock = Util::Concurrency::createLock(*mutex);
		aborted = true;
	}

private:
	Util::Concurrency::Semaphore* sem;
	Util::Concurrency::Mutex* mutex;
	uint32_t time;
	bool timeout;
	bool aborted;

	void run() override {
		uint32_t timer = 0;
		while(timer < time) {
			// I hate this, but i have no idea how to do it otherwise
			Util::Utils::sleep(1);
			++timer;
			{
				auto lock = Util::Concurrency::createLock(*mutex);
				if(aborted)
					break;
			}
		}

		auto lock = Util::Concurrency::createLock(*mutex);
		if(timer >= time && !aborted) {
			timeout = true;
			sem->post();
		}
	}
};

class FileCopier : public Util::Concurrency::UserThread {
private:
	bool closed;
	Util::Concurrency::Semaphore* sem;
	Util::Concurrency::Mutex* mutex;
	typedef std::tuple<std::string, std::string, Tools::asyncCopyCallback_t> QueueEntry_t;
	std::deque<QueueEntry_t> queue;
public:
	FileCopier() : Util::Concurrency::UserThread(), closed(false),
	sem(Util::Concurrency::createSemaphore()), mutex(Util::Concurrency::createMutex()) {start();}

	~FileCopier() {
		close();
		join();
		delete sem;
		delete mutex;
	}
	void close() {
		sem->post();
		closed = true;
	}

	void enqueue(const std::string& src, const std::string& dest, const Tools::asyncCopyCallback_t& callback) {
		auto lock = Util::Concurrency::createLock(*mutex);
		auto entry = std::make_tuple(src, dest, callback);
		queue.push_back(entry);
		sem->post();
	}

	void run() override {
		while(!closed) {
			sem->wait();
			bool empty = false;
			{
				auto lock = Util::Concurrency::createLock(*mutex);
				empty = queue.empty();
			}
			if(!empty) {
				QueueEntry_t entry;
				{
					auto lock = Util::Concurrency::createLock(*mutex);
					entry = queue.front();
					queue.pop_front();
				}

				Util::FileName src(std::get<0>(entry));
				Util::FileName dest(std::get<1>(entry));
				bool success = Util::FileUtils::copyFile(src, dest);
				std::get<2>(entry)(std::get<0>(entry), std::get<1>(entry), success);
			}
		}
	}
};

int32_t Tools::crc32(const std::string& str) {
	union {
		uint32_t u;
		int32_t s;
	} crc;
COMPILER_WARN_PUSH // disabling compiler warning caused by "#define Z_NULL 0" in zlib.h when used as pointer
COMPILER_WARN_OFF(-Wzero-as-null-pointer-constant)
	crc.u = ::crc32(0L, Z_NULL, 0);
COMPILER_WARN_POP
	crc.u = ::crc32(crc.u, reinterpret_cast<const Bytef*>(str.c_str()), str.length());
	return crc.s;
}

static std::vector<uint8_t> receiveData(Util::Network::TCPConnection* connection, uint32_t size, uint32_t& attempts) {
	std::vector<uint8_t> data = connection->receiveData(size);
	while(attempts > 0 && data.empty()) {
		--attempts;
		Utils::sleep(1);
		data = connection->receiveData(size);
	}
	return data;
}

int32_t Tools::generateId() {
	static std::mt19937 gen(static_cast<unsigned int>(std::time(nullptr)));
	static std::uniform_int_distribution<int32_t> distribution(-999999, 999999);
	return distribution(gen);
}

std::vector<uint8_t> Tools::readRawMSG(Util::Network::TCPConnection* connection, uint32_t timeout) {

	uint32_t attempts = timeout;

	// receive length
	const std::vector<uint8_t> length_data = receiveData(connection, sizeof(int32_t), attempts);
	if(length_data.empty()) {
		return std::vector<uint8_t>();
	}
	const int32_t length = Tools::getInt(length_data, 0);
	if(length <= 0)
		return std::vector<uint8_t>();

	// receive body
	return receiveData(connection, static_cast<uint32_t>(length), attempts);
}

Message* Tools::readTCPMSG(Network::TCPConnection* connection, uint32_t timeout) {
	uint32_t attempts = timeout;

	// receive length
	std::vector<uint8_t> data = receiveData(connection, sizeof(int32_t), attempts);
	if(data.empty()) {
		return nullptr;
	}
	int32_t length = Tools::getInt(data, 0);
	if(length < LENGTH_TCP_HEADER) {
		WARN("Message body has wrong size.");
		return nullptr;
	}

	// receive sessionId
	data = receiveData(connection, sizeof(int32_t), attempts);
	if(data.empty()) {
		WARN("Unexpected end of stream.");
		return nullptr;
	}
	int32_t sessionId = Tools::getInt(data, 0);

	// receive orderId
	data = receiveData(connection, sizeof(int64_t), attempts);
	if(data.empty()) {
		WARN("Unexpected end of stream.");
		return nullptr;
	}
	int64_t orderId = Tools::getLong(data, 0);

	// receive type
	data = receiveData(connection, sizeof(int32_t), attempts);
	if(data.empty()) {
		WARN("Unexpected end of stream.");
		return nullptr;
	}
	int32_t type = Tools::getInt(data, 0);

	// receive body
	if(length > LENGTH_TCP_HEADER) {
		data = receiveData(connection, static_cast<uint32_t>(length) - LENGTH_TCP_HEADER, attempts);
		if(data.empty()) {
			WARN("Unexpected end of stream.");
			return nullptr;
		}
	}

	return Message::get(sessionId, orderId, type, data);
}

Message* Tools::readUDPMSG(Util::Network::UDPNetworkSocket* socket) {

	Network::UDPNetworkSocket::Packet* packet = socket->receive();
	if(!packet)
		return nullptr;

	if(packet->packetData.size() < LENGTH_UDP_HEADER+4) {
		WARN("Unexpected end of data package.");
		return nullptr;
	}

	// receive length
	int32_t length = Tools::getInt(packet->packetData, 0);
	if(static_cast<uint32_t>(length) != packet->packetData.size()-4) {
		WARN("Message body has wrong size.");
		return nullptr;
	}

	int32_t clientId = Tools::getInt(packet->packetData, OFFSET_UDP_CLIENTID);
	int32_t sessionId = Tools::getInt(packet->packetData, OFFSET_UDP_SESSIONID);
	int64_t orderId = Tools::getLong(packet->packetData, OFFSET_UDP_ORDER);
	int32_t type = Tools::getInt(packet->packetData, OFFSET_UDP_MSGTYPE);

	const uint8_t * dataBegin = packet->packetData.data() + OFFSET_UDP_BODY;
	const std::vector<uint8_t> body(dataBegin, dataBegin + packet->packetData.size() - LENGTH_UDP_HEADER - 4);

	delete packet;

	return Message::get(clientId, sessionId, orderId, type, body);
}

bool Tools::sendTCPMSG(Network::TCPConnection* connection, Message* msg) {
	int32_t bodyLength = msg->getBodyLength();
	std::vector<uint8_t> data(4 + LENGTH_TCP_HEADER + static_cast<uint32_t>(bodyLength));
	putIntBig(data, OFFSET_LENGTH, LENGTH_TCP_HEADER + msg->getBodyLength());
	putIntBig(data, OFFSET_TCP_SESSIONID, msg->getSession());
	putLongBig(data, OFFSET_TCP_ORDER, msg->getOrder());
	putIntBig(data, OFFSET_TCP_MSGTYPE, msg->getType());
	if(bodyLength > 0) {
		const std::vector<uint8_t> & body = msg->getBody();
		int32_t off = msg->getBodyOffset();
		if(body.empty()) {
			WARN("Message body is empty.");
			return false;
		}
		if(body.size()< static_cast<uint32_t>(off+bodyLength)) {
			WARN("Message body has wrong size.");
			return false;
		}
		std::copy(body.begin() + off, body.begin() + off + bodyLength, data.begin() + OFFSET_TCP_BODY);
	}
	return connection->sendData(data);
}


bool Tools::sendUDPMSG(Network::UDPNetworkSocket* socket, Message* msg) {
	int32_t bodyLength = msg->getBodyLength();
	std::vector<uint8_t> data(4 + LENGTH_UDP_HEADER + static_cast<uint32_t>(bodyLength));
	putIntBig(data, OFFSET_LENGTH, LENGTH_UDP_HEADER + msg->getBodyLength());
	putIntBig(data, OFFSET_UDP_CLIENTID, msg->getClientId());
	putIntBig(data, OFFSET_UDP_SESSIONID, msg->getSession());
	putLongBig(data, OFFSET_UDP_ORDER, msg->getOrder());
	putIntBig(data, OFFSET_UDP_MSGTYPE, msg->getType());
	if(bodyLength > 0) {
		const std::vector<uint8_t> & body = msg->getBody();
		int32_t off = msg->getBodyOffset();
		if(body.empty()) {
			WARN("Message body is empty.");
			return false;
		}
		if(body.size()< static_cast<uint32_t>(off+bodyLength)) {
			WARN("Message body has wrong size.");
			return false;
		}
		std::copy(body.begin() + off, body.begin() + off + bodyLength, data.begin() + OFFSET_UDP_BODY);
	}
	return socket->sendData(data.data(), data.size()) > 0;
}

bool Tools::timedWait(Util::Concurrency::Semaphore* sem, uint32_t timeout) {
	if(sem->tryWait()) {
		return true;
	}
	SemTimeout to(sem, timeout);
	sem->wait();
	to.abort();
	to.join();
	return !to.isTimeout();
}

bool Tools::asyncCopy(const std::string& src, const std::string& dest, const asyncCopyCallback_t& callback) {
	static FileCopier copier;
	copier.enqueue(src, dest, callback);
	return true;
}

}

#endif /* MINSG_EXT_D3FACT */
