#pragma once

#include "MagixObject.h"

class MagixItem : public MagixObject
{
protected:
	unsigned short ID;
	String mesh;
	String particleType;
public:
	bool hasEffect;
	MagixItem(SceneNode *node, const unsigned short &id, const String &meshName);
	~MagixItem();
	const OBJECT_TYPE getType();
	SceneNode* getObjectNode();
	const unsigned short getID();
	const String getMesh();
	const Vector3 getPosition();

	// ОСТАВЬ ТОЛЬКО ОБЪЯВЛЕНИЯ (inline реализации)
	void setParticleType(const String &type) { particleType = type; }
	const String getParticleType() { return particleType; }
};