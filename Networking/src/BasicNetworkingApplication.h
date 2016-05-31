#pragma once

#include "BaseApplication.h"
#include "../ServerApplication/GameMessages.h"
#include "../dep/glm/glm/ext.hpp"
#include "../ServerApplication/GameObject.h"
#include "Gizmos.h"
#include "Camera.h"

#include <vector>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

#include <RakPeerInterface.h>
#include <MessageIdentifiers.h>
#include <BitStream.h>
#include <Rand.h>


enum SyncType
{
	POSITION_ONLY,
	LERP,
	INTERPOLATION
};

typedef unsigned int uint;

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
	void readObjectsFromServer(RakNet::BitStream& bsIn);
	void readObjectVelocityFromServer(RakNet::BitStream& bsIn);

	//Handle GameObjects
	void createGameObject();
	void moveClientObject(float deltatime);
	void sendUpdatedObjectPositionToServer(GameObject& myClientObject);
	void sendObjectVelocityToServer(GameObject& myClientObject);
	void UpdateObjects(float deltatime);

	//handle input
	void handleInput(float deltatime);

private:
	RakNet::RakPeerInterface* m_pPeerInterface;
	const char* IP = "127.0.0.1";
	const unsigned short PORT = 5456;
	uint m_uiClientID;
	uint m_uiClientObjectIndex;
	glm::vec3 m_myColour;
	std::vector<GameObject> m_gameObjects;
	Camera* m_pCamera;


	SyncType m_eSyncType;
};