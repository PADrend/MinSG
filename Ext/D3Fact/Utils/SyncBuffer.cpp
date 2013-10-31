/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/


#ifdef MINSG_EXT_D3FACT

#include "SyncBuffer.h"
#include "../Tools.h"
#include "ConditionVariable.h"

#include <Util/Concurrency/Concurrency.h>
#include <Util/Concurrency/Semaphore.h>
#include <Util/Concurrency/Mutex.h>
#include <Util/Concurrency/Lock.h>

#include <deque>
#include <algorithm>
#include <cstring>

namespace D3Fact {
using namespace Util::Concurrency;

class SyncBuffer::pimpl {
public:
	pimpl() :
		 buffer(), closed(false),
		 mutex(createMutex())  {
		cond = new ConditionVariable(mutex);
	}
	pimpl(const pimpl& src) = delete;
	virtual ~pimpl() {
		delete cond;
		delete mutex;
	}

	std::deque<char> buffer;

	bool closed;

	Mutex* mutex;
	ConditionVariable* cond;
};

SyncBuffer::SyncBuffer() : std::streambuf(), impl(new pimpl()) {
}

SyncBuffer::~SyncBuffer() {
	close();
	delete impl;
}

void SyncBuffer::close() {
	impl->closed = true;
	auto lock = Util::Concurrency::createLock(*(impl->mutex));
	impl->cond->broadcast();
}

SyncBuffer::int_type SyncBuffer::underflow() {
	auto lock = Util::Concurrency::createLock(*(impl->mutex));
	while(impl->buffer.empty()) {
		if(impl->closed)
			return traits_type::eof();
		impl->cond->wait();
	}
	return traits_type::to_int_type(impl->buffer.front());
}

SyncBuffer::int_type SyncBuffer::uflow() {
	int_type c = underflow();
	if(c != traits_type::eof()) {
		auto lock = Util::Concurrency::createLock(*(impl->mutex));
		impl->buffer.pop_front();
	}
	return c;
}

SyncBuffer::int_type SyncBuffer::pbackfail(int_type c) {
	auto lock = Util::Concurrency::createLock(*(impl->mutex));
	if(impl->closed)
		return traits_type::eof();
	impl->buffer.push_front(traits_type::to_char_type(c));
	return c;
}

std::streamsize SyncBuffer::showmanyc() {
	auto lock = Util::Concurrency::createLock(*(impl->mutex));
	return impl->buffer.size();
}

std::streamsize SyncBuffer::xsgetn(char_type* s, std::streamsize count) {
	auto lock = Util::Concurrency::createLock(*(impl->mutex));
	while(impl->buffer.size() < static_cast<size_t>(count) && !impl->closed) {
		impl->cond->wait();
	}
	std::streamsize read = std::min<std::streamsize>(count, impl->buffer.size());
	std::copy(impl->buffer.begin(), impl->buffer.begin() + read, s);
	impl->buffer.erase(impl->buffer.begin(), impl->buffer.begin() + read);
	return read;
}

SyncBuffer::int_type SyncBuffer::overflow(int_type c) {
	if(c == traits_type::eof()) {
		this->close();
	} else {
		auto lock = Util::Concurrency::createLock(*(impl->mutex));
		impl->buffer.push_back(traits_type::to_char_type(c));
		impl->cond->broadcast();
	}
	return c;
}

int SyncBuffer::sync() {
	return 0;
}

std::streamsize SyncBuffer::xsputn(const char_type* s, std::streamsize count) {
	auto lock = Util::Concurrency::createLock(*(impl->mutex));
	impl->buffer.insert(impl->buffer.end(), s, s + count);
	impl->cond->broadcast();
	return count;
}

} /* namespace D3Fact */
#endif /* MINSG_EXT_D3FACT */
