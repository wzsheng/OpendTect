#pragma once
/*+
 * ________________________________________________________________________
 *
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * Author:	  Wayne Mogg
 * Date:	  Apr 2022
 * ________________________________________________________________________
 *
 * -*/


#include "mmprocmod.h"

#include "namedobj.h"
#include "ptrman.h"
#include "uistringset.h"

namespace Network {
    class Authority;
    class RequestConnection;
    class RequestPacket;
    class RequestServer;
    class Service;
}
namespace OD { namespace JSON { class Object; } }

mExpClass(MMProc) MMPServer : public NamedCallBacker
{ mODTextTranslationClass(MMPServer)
public:
			MMPServer(PortNr_Type, int timeout=2000);
			~MMPServer();
			MMPServer(const MMPServer&) = delete;
    MMPServer		operator=(const MMPServer&) = delete;

    uiRetVal		errMsg() const		{ return errmsg_; }
    bool		isOK() const;
    void		setTimeout(int timeout) { timeout_ = timeout; }
    void		setLogFile(const char*);

    uiRetVal		sendResponse(const char* key, const OD::JSON::Object&);

    CNotifier<MMPServer,const OD::JSON::Object&>	startJob;
    CNotifier<MMPServer,const uiRetVal&>		logMsg;
    Notifier<MMPServer>					dataRootChg;
    Notifier<MMPServer>					getLogFile;

protected:
    Network::RequestServer&		server_;
    Network::Service&			thisservice_;
    int					timeout_;
    Network::RequestConnection*		tcpconn_ = nullptr;
    RefMan<Network::RequestPacket>	packet_;
    uiRetVal				errmsg_;

    void		handleStatusRequest(const OD::JSON::Object&);
    void		handleSetDataRootRequest(const OD::JSON::Object&);

    void		newConnectionCB(CallBacker*);
    void		packetArrivedCB(CallBacker*);
    void		connClosedCB(CallBacker*);

    uiRetVal		doHandleRequest(const OD::JSON::Object&);

};
