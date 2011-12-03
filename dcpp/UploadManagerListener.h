#ifndef DCPLUSPLUS_DCPP_UPLOADMANAGERLISTENER_H_
#define DCPLUSPLUS_DCPP_UPLOADMANAGERLISTENER_H_

#include "forward.h"
#include "typedefs.h"

#include "noexcept.h"

namespace dcpp {

class UploadManagerListener {
public:
	virtual ~UploadManagerListener() { }
	template<int I>	struct X { enum { TYPE = I }; };

	typedef X<0> Complete;
	typedef X<1> Failed;
	typedef X<2> Starting;
	typedef X<3> Tick;
	typedef X<4> WaitingAddFile;
	typedef X<5> WaitingRemoveUser;

	virtual void on(Starting, Upload*) noexcept { }
	virtual void on(Tick, const UploadList&) noexcept { }
	virtual void on(Complete, Upload*) noexcept { }
	virtual void on(Failed, Upload*, const string&) noexcept { }
	virtual void on(WaitingAddFile, const HintedUser&, const string&) noexcept { }
	virtual void on(WaitingRemoveUser, const HintedUser&) noexcept { }

};

} // namespace dcpp

#endif /*DCPLUSPLUS_DCPP_UPLOADMANAGERLISTENER_H_*/
