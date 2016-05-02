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
				GameObject newGameObject;

				bsIn.Read(newGameObject.fXPos);
				bsIn.Read(newGameObject.fZPos);
				bsIn.Read(newGameObject.uiObjectID);
				m_gameObjects[newGameObject.uiObjectID].fXPos = newGameObject.fXPos;
				m_gameObjects[newGameObject.uiObjectID].fZPos = newGameObject.fZPos;

				RakNet::SystemAddress unassignedAddress = RakNet::UNASSIGNED_SYSTEM_ADDRESS;
				sendGameObjectToAllClients(newGameObject, unassignedAddress);
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
	RakNet::SystemAddress unassignedAddress = RakNet::UNASSIGNED_SYSTEM_ADDRESS;
	sendGameObjectToAllClients(newGameObject, unassignedAddress);
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