#include "Server.h"

Server::Server() {
	// initialize the Raknet peer interface first
	m_pPeerInterface = RakNet::RakPeerInterface::GetInstance();

	m_uiConnectionCounter = 1;
	m_uiObjectCounter = 1;
}

Server::~Server() {

}

void Server::run() {

	// startup the server, and start it listening to clients
	std::cout << "Starting up the server..." << std::endl;

	// create a socket descriptor to describe this connection
	RakNet::SocketDescriptor sd(PORT, 0);

	// now call startup - max of 32 connections, on the assigned port
	m_pPeerInterface->Startup(32, &sd, 1);
	m_pPeerInterface->SetMaximumIncomingConnections(32);

	std::cout << "Startup complete." << std::endl;

	handleNetworkMessages();
}

void Server::handleNetworkMessages() {
	RakNet::Packet* pPacket = nullptr;

	while (true) {
		for (	pPacket = m_pPeerInterface->Receive();
				pPacket;
				m_pPeerInterface->DeallocatePacket(pPacket), pPacket = m_pPeerInterface->Receive()) {

			switch (pPacket->data[0]) {
			case ID_NEW_INCOMING_CONNECTION: {
				addNewConnection(pPacket->systemAddress);
				std::cout << "A connection is incoming.\n";
				break;
			}
			case ID_DISCONNECTION_NOTIFICATION:
				std::cout << "A client has disconnected.\n";
				removeConnection(pPacket->systemAddress);
				break;
			case ID_CONNECTION_LOST:
				std::cout << "A client lost the connection.\n";
				removeConnection(pPacket->systemAddress);
				break;
			case ID_CLIENT_CREATE_OBJECT:
			{
				RakNet::BitStream bsIn(pPacket->data, pPacket->length, false);
				bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
				createNewObject(bsIn, pPacket->systemAddress);
				break;
			}
			case ID_CLIENT_UPDATE_OBJECT_POSITION:
			{
				RakNet::BitStream bsIn(pPacket->data, pPacket->length, false);
				bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
				moveGameObject(bsIn, pPacket->systemAddress);
				break;
			}
			default:
				std::cout << "Received a message with a unknown ID: " << pPacket->data[0];
				break;
			}
		}
	}
}

void Server::addNewConnection(RakNet::SystemAddress systemAddress) {
	ConnectionInfo info;
	info.sysAddress = systemAddress;
	info.uiConnectionID = m_uiConnectionCounter++;
	m_connectedClients[info.uiConnectionID] = info;

	sendClientIDToClient(info.uiConnectionID);
}

void Server::removeConnection(RakNet::SystemAddress systemAddress) {
	for (auto it = m_connectedClients.begin(); it != m_connectedClients.end(); it++) {
		if (it->second.sysAddress == systemAddress) {
			m_connectedClients.erase(it);
			break;
		}
	}
}

unsigned int Server::systemAddressToClientID(RakNet::SystemAddress& systemAddress) {
	for (auto it = m_connectedClients.begin(); it != m_connectedClients.end(); it++) {
		if (it->second.sysAddress == systemAddress) {
			return it->first;
		}
	}

	return 0;
}

void Server::sendClientIDToClient(unsigned int uiClientID) {
	RakNet::BitStream bs;
	bs.Write((RakNet::MessageID)GameMessages::ID_SERVER_CLIENT_ID);
	bs.Write(uiClientID);

	m_pPeerInterface->Send(&bs, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, m_connectedClients[uiClientID].sysAddress, false);
}

void Server::createNewObject(RakNet::BitStream& bsIn, RakNet::SystemAddress& ownerSysAddress)
{
	GameObject newGameObject;

	bsIn.Read(newGameObject.fXPos);
	bsIn.Read(newGameObject.fZPos);
	bsIn.Read(newGameObject.fRedColour);
	bsIn.Read(newGameObject.fGreenColour);
	bsIn.Read(newGameObject.fBlueColour);

	newGameObject.uiOwnerClientID = systemAddressToClientID(ownerSysAddress);
	newGameObject.uiObjectID = m_uiConnectionCounter++;

	m_gameObjects.push_back(newGameObject);

	for (int i = 0; i < m_connectedClients.size(); i++)
	{
		if (m_connectedClients[i].uiConnectionID != newGameObject.uiOwnerClientID)
		{
			sendGameObjectToAllClients(newGameObject, m_connectedClients[i].sysAddress);
		}
	}
	
}

void Server::sendGameObjectToAllClients(GameObject& gameObject, RakNet::SystemAddress& ownerSystemAddress)
{
	RakNet::BitStream bsOut;
	bsOut.Write((RakNet::MessageID)GameMessages::ID_SERVER_FULL_OBJECT_DATA);
	bsOut.Write(gameObject.fXPos);
	bsOut.Write(gameObject.fZPos);
	bsOut.Write(gameObject.fRedColour);
	bsOut.Write(gameObject.fGreenColour);
	bsOut.Write(gameObject.fBlueColour);
	bsOut.Write(gameObject.uiOwnerClientID);
	bsOut.Write(gameObject.uiObjectID);

	m_pPeerInterface->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, ownerSystemAddress, true);
}

void Server::moveGameObject(RakNet::BitStream& bsIn, RakNet::SystemAddress& ownerSystemAddress)
{
	GameObject myClientObject;
	bsIn.Read(myClientObject);

	for (int i = 0; i < m_gameObjects.size(); i++)
	{
		if (m_gameObjects[i].uiObjectID == myClientObject.uiObjectID)
		{
			m_gameObjects[i] = myClientObject;

			for (int j = 0; j < m_connectedClients.size(); j++)
			{
				if (m_connectedClients[j].uiConnectionID != m_gameObjects[i].uiOwnerClientID)
				{
					sendGameObjectToAllClients(m_gameObjects[i], m_connectedClients[j].sysAddress);
				}
			}
		}
	}
}