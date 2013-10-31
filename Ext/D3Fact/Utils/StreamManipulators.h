/*
	This file is part of the MinSG library extension D3Fact.
	Copyright (C) 2010-2012 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/


#ifdef MINSG_EXT_D3FACT

#ifndef STREAMMANIPULATORS_H_
#define STREAMMANIPULATORS_H_

#include "SyncBuffer.h"

namespace D3Fact {

template<typename _CharT, typename _Traits>
inline std::basic_ostream<_CharT, _Traits>& closestream(std::basic_ostream<_CharT, _Traits>& __os) {
	SyncBuffer* buf = dynamic_cast<SyncBuffer*>(__os.rdbuf());
	if(buf)
		buf->close();
	return __os.flush();
}

}  // namespace D3Fact

#endif /* STREAMMANIPULATORS_H_ */
#endif /* MINSG_EXT_D3FACT */
