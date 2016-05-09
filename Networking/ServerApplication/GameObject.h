#pragma once
#include "../dep/glm/glm/ext.hpp"

enum SyncType
{
	POSITION_ONLY,
	LERP,
	INTERPOLATION
};

struct GameObject {
	unsigned int uiOwnerClientID;
	unsigned int uiObjectID;

	float fRedColour;
	float fGreenColour;
	float fBlueColour;

	float fForce;

	glm::vec3 position;
	float fAcceleration;
	glm::vec2 Velocity;

	SyncType eSyncType;

	bool bUpdatedObjectPostion;

};
