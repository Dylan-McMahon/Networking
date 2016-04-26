#pragma once

#include "BaseApplication.h"

namespace RakNet {
	class RakPeerInterface;
}

class BasicNetworkingApplication : public BaseApplication {
public:
	BasicNetworkingApplication();
	virtual ~BasicNetworkingApplication();

	virtual bool startup();
	virtual void shutdown();

	virtual bool update(float deltaTime);

	virtual void draw();

	//Initialize the connection
	void HandleNetworkConnection();
	void InitalizeClientConnection();

	//Handle incoming packets
	void HandleNetworkMessages();

private:
	RakNet::RakPeerInterface* m_pPeerInterface;
	const char* IP = "127.0.0.1";
	const unsigned short PORT = 5456;
};