/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_D3FACT

#ifndef TOOLS_H_
#define TOOLS_H_

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>
#include <limits>
#include <functional>

#include <Util/Macros.h>

namespace Util {
namespace Network {
class TCPConnection;
class UDPNetworkSocket;
}
namespace Concurrency {
class Semaphore;
}
}

namespace D3Fact {

class Message;

class Tools {
public:
	static const uint32_t TIMEOUT;

	static int32_t generateId();

	// Conversion

	static inline void putBoolean(std::vector<uint8_t> & data, uint32_t offset, bool value) {
		data[offset] = value ? 1 : 0;
	}

	static inline void putByte(std::vector<uint8_t> & data, uint32_t offset, int8_t value) {
		data[offset] = static_cast<uint8_t>(value);
	}

	static inline void putByte(std::vector<uint8_t> & data, uint32_t offset, int32_t value) {
		data[offset] = static_cast<uint8_t>(value & 0xff);
	}

	static inline void putIntBig(std::vector<uint8_t> & data, uint32_t offset, int32_t value) {
		data[offset+3] = static_cast<uint8_t>((value >>  0) & 0xff);
		data[offset+2] = static_cast<uint8_t>((value >>  8) & 0xff);
		data[offset+1] = static_cast<uint8_t>((value >> 16) & 0xff);
		data[offset+0] = static_cast<uint8_t>((value >> 24) & 0xff);
	}

	static inline void putLongBig(std::vector<uint8_t> & data, uint32_t offset, int64_t value) {
		data[offset+7] = static_cast<uint8_t>((value >>  0) & 0xff);
		data[offset+6] = static_cast<uint8_t>((value >>  8) & 0xff);
		data[offset+5] = static_cast<uint8_t>((value >> 16) & 0xff);
		data[offset+4] = static_cast<uint8_t>((value >> 24) & 0xff);
		data[offset+3] = static_cast<uint8_t>((value >> 32) & 0xff);
		data[offset+2] = static_cast<uint8_t>((value >> 40) & 0xff);
		data[offset+1] = static_cast<uint8_t>((value >> 48) & 0xff);
		data[offset+0] = static_cast<uint8_t>((value >> 56) & 0xff);
	}

	static inline void putFloatBig(std::vector<uint8_t> & data, uint32_t offset, float value) {
		union {
			float f;
			int32_t i;
		} val;
		val.f = value;
		putIntBig(data, offset, val.i);
	}

	static inline void putDoubleBig(std::vector<uint8_t> & data, uint32_t offset, double value) {
		union {
			double d;
			int64_t i;
		} val;
		val.d = value;
		putLongBig(data, offset, val.i);
	}

	static inline void putString(std::vector<uint8_t> & data, uint32_t offset, const std::string & value) {
		putIntBig(data, offset, static_cast<int32_t>(value.size()));
		std::copy(value.data(), value.data()+value.size(), data.data() + offset + sizeof(int32_t));
	}

	static inline void appendBoolean(std::vector<uint8_t> & data, bool value) {
		std::vector<uint8_t> temp(sizeof(value));
		putBoolean(temp,0,value);
		data.insert(data.end(), temp.begin(), temp.end());
	}

	static inline void appendByte(std::vector<uint8_t> & data, int8_t value) {
		std::vector<uint8_t> temp(sizeof(value));
		putByte(temp,0,value);
		data.insert(data.end(), temp.begin(), temp.end());
	}

	static inline void appendByte(std::vector<uint8_t> & data, int32_t value) {
		std::vector<uint8_t> temp(sizeof(int8_t));
		putByte(temp,0,value);
		data.insert(data.end(), temp.begin(), temp.end());
	}

	static inline void appendIntBig(std::vector<uint8_t> & data, int32_t value) {
		std::vector<uint8_t> temp(sizeof(value));
		putIntBig(temp,0,value);
		data.insert(data.end(), temp.begin(), temp.end());
	}

	static inline void appendLongBig(std::vector<uint8_t> & data, int64_t value) {
		std::vector<uint8_t> temp(sizeof(value));
		putLongBig(temp,0,value);
		data.insert(data.end(), temp.begin(), temp.end());
	}

	static inline void appendFloatBig(std::vector<uint8_t> & data, float value) {
		std::vector<uint8_t> temp(sizeof(value));
		putFloatBig(temp,0,value);
		data.insert(data.end(), temp.begin(), temp.end());
	}

	static inline void appendDoubleBig(std::vector<uint8_t> & data, double value) {
		std::vector<uint8_t> temp(sizeof(value));
		putDoubleBig(temp,0,value);
		data.insert(data.end(), temp.begin(), temp.end());
	}

	static inline void appendString(std::vector<uint8_t> & data, const std::string& value) {
		std::vector<uint8_t> temp(sizeof(int32_t)+value.size());
		putString(temp, 0, value);
		data.insert(data.end(), temp.begin(), temp.end());
	}

	static inline bool getBoolean(const std::vector<uint8_t> & data, uint32_t offset) {
		return data[offset] > 0;
	}

	static inline int8_t getByte(const std::vector<uint8_t> & data, uint32_t offset) {
		return static_cast<int8_t>(data[offset]);
	}

	static inline int32_t getInt(const std::vector<uint8_t> & data, uint32_t offset) {
		return 	  ((data[offset + 3] & 0xff) <<  0)
				+ ((data[offset + 2] & 0xff) <<  8)
				+ ((data[offset + 1] & 0xff) << 16)
				+ ((data[offset + 0] & 0xff) << 24);
	}

	static inline int64_t getLong(const std::vector<uint8_t> & data, uint32_t offset) {
		return 	  ((static_cast<int64_t>(data[offset + 7] & 0xff)) <<  0)
				+ ((static_cast<int64_t>(data[offset + 6] & 0xff)) <<  8)
				+ ((static_cast<int64_t>(data[offset + 5] & 0xff)) << 16)
				+ ((static_cast<int64_t>(data[offset + 4] & 0xff)) << 24)
				+ ((static_cast<int64_t>(data[offset + 3] & 0xff)) << 32)
				+ ((static_cast<int64_t>(data[offset + 2] & 0xff)) << 40)
				+ ((static_cast<int64_t>(data[offset + 1] & 0xff)) << 48)
				+ ((static_cast<int64_t>(data[offset + 0] & 0xff)) << 56);
	}

	static inline float getFloat(const std::vector<uint8_t> & data, uint32_t offset) {
		union {
			float f;
			int32_t i;
		} val;
		val.i = getInt(data, offset);
		return val.f;
	}

	static inline double getDouble(const std::vector<uint8_t> & data, uint32_t offset) {
		union {
			double d;
			int64_t i;
		} val;
		val.i = getLong(data, offset);
		return val.d;
	}

	static inline std::string getString(const std::vector<uint8_t> & data, uint32_t offset) {
		uint32_t length = static_cast<uint32_t>(getInt(data, offset));
		if(length > data.size()-offset-sizeof(length))
			length = data.size()-offset-sizeof(length);
		return std::string(data.begin() + offset + sizeof(length), data.begin() + offset + sizeof(length) + length);
	}

	static int32_t crc32(const std::string& str);

	// Message handling

	static std::vector<uint8_t> readRawMSG(Util::Network::TCPConnection* connection, uint32_t timeout=Tools::TIMEOUT);
	static Message* readTCPMSG(Util::Network::TCPConnection* connection, uint32_t timeout=Tools::TIMEOUT);
	static Message* readUDPMSG(Util::Network::UDPNetworkSocket* socket);

	static bool sendTCPMSG(Util::Network::TCPConnection* connection, Message* msg);
	static bool sendUDPMSG(Util::Network::UDPNetworkSocket* socket, Message* msg);

	// utils

	static bool timedWait(Util::Concurrency::Semaphore* sem, uint32_t timeout=Tools::TIMEOUT);

	typedef std::function<void(const std::string& src, const std::string& dest, bool success)> asyncCopyCallback_t;
	static bool asyncCopy(const std::string& src, const std::string& dest, const asyncCopyCallback_t& callback);
};

}

#endif /* TOOLS_H_ */
#endif /* MINSG_EXT_D3FACT */
