#pragma once
#include "../dep/glm/glm/ext.hpp"


struct GameObject {
	unsigned int uiOwnerClientID;
	unsigned int uiObjectID;

	float fRedColour;
	float fGreenColour;
	float fBlueColour;

	float fForce;

	glm::vec3 position;
	glm::vec3 newPosition;
	float fAcceleration;
	glm::vec2 Velocity;
	float timeStamp;


	bool bUpdatedObjectVelocity;
	bool bUpdatedObjectPosition;

};
