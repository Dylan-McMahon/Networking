#include <iostream>
#include <string>

//Raknet Includes
#include <RakPeerInterface.h>
#include <MessageIdentifiers.h>
#include <BitStream.h>

typedef unsigned short uShort;
void HandleNetworkMessages(RakNet::RakPeerInterface* pPeerInterface);

void main() {

	const uShort PORT = 5456;
	RakNet::RakPeerInterface* pPeerInterface = nullptr;
	//
	//Server Setup
	//
	std::cout << "Starting up the server..." << std::endl;
	//initialize the peer interface
	pPeerInterface = RakNet::RakPeerInterface::GetInstance();
	//craete socket description
	RakNet::SocketDescriptor sd(PORT, 0);
	//Startup the server - max 32 conections
	pPeerInterface->Startup(32, &sd, 1);
	pPeerInterface->SetMaximumIncomingConnections(32);
	//
	//End startup
	//

	//Listen for a connection
	HandleNetworkMessages(pPeerInterface);

}

void HandleNetworkMessages(RakNet::RakPeerInterface* pPeerInterface)
{
	//Handles Network Messages for the server
	RakNet::Packet* pPacket = nullptr;

	while (true)
	{
		for (pPacket = pPeerInterface->Receive(); pPacket; pPeerInterface->DeallocatePacket(pPacket), pPacket = pPeerInterface->Receive())
		{
			switch (pPacket->data[0])
			{
			case ID_NEW_INCOMING_CONNECTION:
				std::cout << "A connection is incoming. \n";
				break;
			case ID_DISCONNECTION_NOTIFICATION:
				std::cout << "A client has disconnected. \n";
				break;
			case ID_CONNECTION_LOST:
				std::cout << "A client has lost connection. \n";
				break;
			default:
				std::cout << "Received a message with a unknown id: " << pPacket->data[0];
				break;
			}
		}
	}
}
