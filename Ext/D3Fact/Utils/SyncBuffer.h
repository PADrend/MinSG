/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/


#ifdef MINSG_EXT_D3FACT

#ifndef SYNCBUFFER_H_
#define SYNCBUFFER_H_

#include <memory>
#include <streambuf>

namespace D3Fact {

class SyncBuffer : public std::streambuf {
public:
	SyncBuffer();
	SyncBuffer(const SyncBuffer &) = delete;
	SyncBuffer & operator = (const SyncBuffer &) = delete;

	virtual ~SyncBuffer();

	void close();
private:
	int_type underflow() override;
	int_type uflow() override;
	int_type pbackfail(int_type c) override;
	std::streamsize showmanyc() override;
	std::streamsize xsgetn( char_type* s, std::streamsize count ) override;

	int_type overflow(int_type c) override;
	int sync() override;
	std::streamsize xsputn( const char_type* s, std::streamsize count ) override;

	class pimpl;
	std::unique_ptr<pimpl> impl;
};

} /* namespace D3Fact */
#endif /* SYNCBUFFER_H_ */
#endif /* MINSG_EXT_D3FACT */
