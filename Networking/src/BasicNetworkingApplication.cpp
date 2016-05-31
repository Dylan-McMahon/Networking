#include "BasicNetworkingApplication.h"

BasicNetworkingApplication::BasicNetworkingApplication() {
	m_eSyncType = POSITION_ONLY;
	m_uiClientObjectIndex = -1;
}

BasicNetworkingApplication::~BasicNetworkingApplication() {

}

bool BasicNetworkingApplication::startup() {
	/// setup the basic window
	createWindow("Client Application", 1280, 720);
	Gizmos::create();
	m_pCamera = new Camera(glm::pi<float>() * 0.25f, 16 / 9.f, 0.1f, 1000.f);
	m_pCamera->setLookAtFrom(glm::vec3(10), glm::vec3(0, 1, 0));
	m_myColour = glm::vec3((static_cast <float> (rand()) / static_cast <float> (RAND_MAX)), 
						   (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)), 
						   (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)));
	HandleNetworkConnection();
	return true;
}

void BasicNetworkingApplication::shutdown() {

}

bool BasicNetworkingApplication::update(float deltaTime) {
	/// close the application if the window closes
	if (glfwWindowShouldClose(m_window) ||
		glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		return false;
	HandleNetworkMessages();
	m_pCamera->update(deltaTime);
	if (m_gameObjects.size() != 0)
	{
		handleInput(deltaTime);
		moveClientObject(deltaTime);
		UpdateObjects(deltaTime);
	}
	return true;
}

void BasicNetworkingApplication::draw() {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	Gizmos::clear();
	for (int i = 0; i < m_gameObjects.size(); i++)
	{
		GameObject& obj = m_gameObjects[i];
		Gizmos::addSphere(glm::vec3(obj.position.x, 2, obj.position.z), 2, 32, 32, glm::vec4(obj.fRedColour, obj.fGreenColour, obj.fBlueColour, 1), nullptr);
	}
	Gizmos::draw(m_pCamera->getProjectionView());
}

void BasicNetworkingApplication::HandleNetworkConnection()
{
	///Initiialize the Raknet peer interface first
	m_pPeerInterface = RakNet::RakPeerInterface::GetInstance();
	InitalizeClientConnection();
}

void BasicNetworkingApplication::InitalizeClientConnection()
{
	///Craete a socet description 
	///no data needed to conect to a server
	RakNet::SocketDescriptor sd;
	///Call startup - Max connections 1(connection to the server)
	m_pPeerInterface->Startup(1, &sd, 1);
	std::cout << "Connecting to server at: " << IP << std::endl;
	///call raknet connect
	RakNet::ConnectionAttemptResult res = m_pPeerInterface->Connect(IP, PORT, nullptr, 0);
	///Connection check
	if (res != RakNet::CONNECTION_ATTEMPT_STARTED)
	{
		std::cout << "Unable to start connection, Error number: " << res << std::endl;
	}
}

void BasicNetworkingApplication::HandleNetworkMessages()
{
	RakNet::Packet* pPacket;
	for (pPacket = m_pPeerInterface->Receive(); pPacket; m_pPeerInterface->DeallocatePacket(pPacket), pPacket = m_pPeerInterface->Receive())
	{
		switch (pPacket->data[0])
		{
		case ID_REMOTE_DISCONNECTION_NOTIFICATION:
			std::cout << "Another client has disconnected. \n";
			break;
		case ID_REMOTE_CONNECTION_LOST:
			std::cout << "Another client has lost connection. \n";
			break;
		case ID_REMOTE_NEW_INCOMING_CONNECTION:
			std::cout << "Another client has connected. \n";
			break;
		case ID_CONNECTION_REQUEST_ACCEPTED:
			std::cout << "Our connection request has been accepted. \n";
			break;
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			std::cout << "The server is full. \n";
			break;
		case ID_DISCONNECTION_NOTIFICATION:
			std::cout << "We have been disconnected. \n";
			break;
		case ID_CONNECTION_LOST:
			std::cout << "Connection lost. \n";
			break;
		case ID_SERVER_TEXT_MESSAGE:
		{
			RakNet::BitStream bsIn(pPacket->data, pPacket->length, false);
			bsIn.IgnoreBytes(sizeof(RakNet::MessageID));

			RakNet::RakString str;
			bsIn.Read(str);
			std::cout << str.C_String() << std::endl;
			break;
		}
		case ID_SERVER_CLIENT_ID:
		{
			RakNet::BitStream bsIn(pPacket->data, pPacket->length, false);
			bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
			bsIn.Read(m_uiClientID);

			std::cout << "Server has given us an ID of: " << m_uiClientID << std::endl;

			createGameObject();

			break;
		}
		case ID_SERVER_FULL_OBJECT_DATA:
		{
				RakNet::BitStream bsIn(pPacket->data, pPacket->length, false);
				bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
				readObjectsFromServer(bsIn);
				break;
		}
		case ID_SERVER_VELOCITY_OBJECT_DATA:
		{
			if (m_eSyncType != POSITION_ONLY)
			{
				RakNet::BitStream bsIn(pPacket->data, pPacket->length, false);
				bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
				readObjectVelocityFromServer(bsIn);
			}
			break;
		}
		default:
			std::cout << "Received a message with a unknown id: " << pPacket->data[0];
			break;
		}
	}

}

void BasicNetworkingApplication::readObjectsFromServer(RakNet::BitStream& bsIn)
{
	///Create temp object that we will pull all the objects data into
	GameObject tempGameObject;
	///Read in object data
	bsIn.Read(tempGameObject);
	///Check to so if it exists already
	bool bFound = false;
	for (int i = 0; i < m_gameObjects.size(); i++)
	{
		if (m_gameObjects[i].uiObjectID == tempGameObject.uiObjectID)
		{
			bFound = true;
			//Update the object
			GameObject& obj = m_gameObjects[i];
			obj.bUpdatedObjectVelocity =	tempGameObject.bUpdatedObjectVelocity;
			obj.bUpdatedObjectPosition =	tempGameObject.bUpdatedObjectPosition;
			obj.Velocity =					tempGameObject.Velocity;
			obj.newPosition =				tempGameObject.position;

			obj = tempGameObject;
		}
	}

	///If it doesnt exist create it.
	if (!bFound)
	{
		m_gameObjects.push_back(tempGameObject);
		if (tempGameObject.uiOwnerClientID == m_uiClientID)
		{
			m_uiClientObjectIndex = m_gameObjects.size() - 1;
		}
	}
}

void BasicNetworkingApplication::readObjectVelocityFromServer(RakNet::BitStream & bsIn)
{
	unsigned int uiObjectID;
	unsigned int uiConnectionID;
	glm::vec2    vVelocity;

	bsIn.Read(uiObjectID);
	bsIn.Read(uiConnectionID);
	bsIn.Read(vVelocity);

	if (uiConnectionID == m_uiClientID) return;

	for (int i = 0; i < m_gameObjects.size(); i++)
	{
		if (m_gameObjects[i].uiObjectID == uiObjectID)
		{
			m_gameObjects[i].Velocity = vVelocity;
			break;
		}
	}
}


void BasicNetworkingApplication::createGameObject()
{
	///Tell the server to create an object
	RakNet::BitStream bsOut;

	GameObject tempGameObject;
	tempGameObject.uiOwnerClientID = m_uiClientID;
	tempGameObject.position.x = 0.0f;
	tempGameObject.position.z = 0.0f;
	tempGameObject.fRedColour = m_myColour.r;
	tempGameObject.fGreenColour = m_myColour.g;
	tempGameObject.fBlueColour = m_myColour.b;
	tempGameObject.fForce = 2.0f;	
	tempGameObject.bUpdatedObjectVelocity = false;
	tempGameObject.bUpdatedObjectPosition = false;
	tempGameObject.Velocity = glm::vec2(0);
	tempGameObject.newPosition = glm::vec3(0);

	///Copy the data to a packet
	bsOut.Write((RakNet::MessageID)GameMessages::ID_CLIENT_CREATE_OBJECT);
	bsOut.Write(tempGameObject);

	///Send the data
	m_pPeerInterface->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
}

void BasicNetworkingApplication::moveClientObject(float deltatime)
{
	///Check Movement

	///Move the object

	///Check Object validation
	if (m_uiClientObjectIndex == -1) return;
	if(m_gameObjects.size() == 0) return;
	//Check Movement
	GameObject& myClientObject = m_gameObjects[m_uiClientObjectIndex];
	
	if (myClientObject.bUpdatedObjectVelocity)
	{
		sendObjectVelocityToServer(myClientObject);
		myClientObject.bUpdatedObjectVelocity = false;
	}

	static float positionSendTimer = 0.0f;
	positionSendTimer += deltatime;
	if (positionSendTimer > 0.5f)
	{
		myClientObject.bUpdatedObjectPosition = true;
		myClientObject.newPosition = myClientObject.position;
		sendUpdatedObjectPositionToServer(myClientObject);
		positionSendTimer = 0.0f;
	}
}

void BasicNetworkingApplication::sendUpdatedObjectPositionToServer(GameObject& myClientObject)
{
	RakNet::BitStream bsOut;
	bsOut.Write((RakNet::MessageID)GameMessages::ID_CLIENT_UPDATE_OBJECT_POSITION);
	bsOut.Write(myClientObject);
	m_pPeerInterface->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
}

void BasicNetworkingApplication::handleInput(float deltatime)
{
	if (m_uiClientObjectIndex == -1) return;

	GameObject& myClientObject = m_gameObjects[m_uiClientObjectIndex];

	if (glfwGetKey(m_window, GLFW_KEY_I))
	{
		myClientObject.Velocity = glm::vec2(0, 4);
		myClientObject.bUpdatedObjectVelocity = true;
	}
	if (glfwGetKey(m_window, GLFW_KEY_K))
	{
		myClientObject.Velocity = glm::vec2(0, -4);
		myClientObject.bUpdatedObjectVelocity = true;
	}

	if (glfwGetKey(m_window, GLFW_KEY_1))
	{
		m_eSyncType = POSITION_ONLY;
	}
	if (glfwGetKey(m_window, GLFW_KEY_2))
	{
		m_eSyncType = LERP;
	}
	if (glfwGetKey(m_window, GLFW_KEY_3))
	{
		m_eSyncType = INTERPOLATION;
	}
}

void BasicNetworkingApplication::sendObjectVelocityToServer(GameObject& myClientObject)
{
	///Check Object validation
	if (m_gameObjects.size() == 0) return;

	RakNet::BitStream bsOut;
	bsOut.Write((RakNet::MessageID)GameMessages::ID_CLIENT_UPDATE_OBJECT_VELOCITY);
	bsOut.Write(myClientObject.uiObjectID);
	bsOut.Write(myClientObject.uiOwnerClientID);
	bsOut.Write(myClientObject.Velocity);
	m_pPeerInterface->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
}

void BasicNetworkingApplication::UpdateObjects(float deltatime)
{
	if (m_eSyncType == INTERPOLATION)
	{
		for (int i = 0; i < m_gameObjects.size(); i++)
		{
			if (m_gameObjects[i].bUpdatedObjectPosition == true && m_gameObjects[i].uiOwnerClientID != m_uiClientID)
			{
				float time = timeGetTime();
				time -= m_gameObjects[i].timeStamp;
				m_gameObjects[i].newPosition = m_gameObjects[i].position + glm::vec3(m_gameObjects[i].Velocity.x, 0, m_gameObjects[i].Velocity.y) * time;
				m_gameObjects[i].position += glm::lerp(m_gameObjects[i].position, m_gameObjects[i].newPosition, deltatime) / deltatime;
			}
			else
			{
				m_gameObjects[i].position += glm::vec3(m_gameObjects[i].Velocity.x, 0, m_gameObjects[i].Velocity.y) * deltatime;
			}
			m_gameObjects[i].bUpdatedObjectPosition = false;
		}
	}
	else
	{
		for (int i = 0; i < m_gameObjects.size(); i++)
		{
			m_gameObjects[i].position += glm::vec3(m_gameObjects[i].Velocity.x, 0, m_gameObjects[i].Velocity.y) * deltatime;
		}
	}
}