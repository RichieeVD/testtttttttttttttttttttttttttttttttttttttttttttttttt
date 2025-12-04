#include "MagixEffectsManager.h"
#include "MagixConst.h"

void MagixEffectsManager::initialize(SceneManager *sceneMgr)
{
	mSceneMgr = sceneMgr;
}

void MagixEffectsManager::reset(bool ignoreUnresettable)
{
	for (int i = 0; i<MAX_RIBBONTRAILS; i++)destroyRibbonTrail(i);
	for (int i = 0; i<MAX_PARTICLES; i++)if (ignoreUnresettable || !particleUnresettable[i])destroyParticle(i);
	for (int i = 0; i<MAX_WATERRIPPLES; i++)destroyWaterRipple(i);
	for (int i = 0; i<MAX_BREATHERS; i++)destroyBreather(i);
	for (int i = 0; i<MAX_ITEMSPARKLE; i++)destroyItemParticle(i);
	if (ignoreUnresettable)for (int i = 0; i<MAX_ITEMEFFECTS; i++)destroyItemEffect(i);
}

void MagixEffectsManager::update(const FrameEvent &evt)
{
	// ОБНОВЛЕНИЕ PARTICLE SYSTEMS
	for (int i = 0; i<MAX_PARTICLES; i++)
	{
		if (mParticle[i])
		{
			//cease emissions when duration is up
			if (particleDuration[i]>0)
			{
				particleDuration[i] -= evt.timeSinceLastFrame;
				if (particleDuration[i] <= 0)
				{
					particleDuration[i] = 0;
					mParticle[i]->getEmitter(0)->setEmissionRate(0);
				}
			}
			//destroy particle system after all particles are gone
			else if (particleDuration[i] == 0 && mParticle[i]->getNumParticles() == 0)
			{
				destroyParticle(i);
			}
		}
	}

	// ОБНОВЛЕНИЕ WATER RIPPLES
	for (int i = 0; i<MAX_WATERRIPPLES; i++)
	{
		if (mWaterRipple[i])
		{
			//Update position
			if (mWaterRippleOwner[i])
				mWaterRippleNode[i]->setPosition(mWaterRippleOwner[i]->getPosition().x, mWaterRippleNode[i]->getPosition().y, mWaterRippleOwner[i]->getPosition().z);
			//Destroy afterall all particles are gone
			if (mWaterRipple[i]->getEmitter(0)->getEmissionRate() == 0 && mWaterRipple[i]->getNumParticles() == 0)
			{
				destroyWaterRipple(i);
			}
		}
	}

	// ОБНОВЛЕНИЕ BREATHERS
	for (int i = 0; i<MAX_BREATHERS; i++)
	{
		if (mBreather[i] && mBreatherOwner[i])
		{
			SceneNode* tBreatherNode = mSceneMgr->getSceneNode("BreatherNode" + StringConverter::toString(i));
			Bone *tBone = mBreatherOwner[i]->getSkeleton()->getBone("Mouth");
			tBreatherNode->setOrientation(tBone->_getDerivedOrientation());
			tBreatherNode->setPosition(tBone->_getDerivedPosition() + tBone->_getDerivedOrientation()*breatherOffset[i]);
		}
	}

	// ВАЖНО: ОБНОВЛЯЕМ ВСЕ эффекты предметов каждый кадр
	for (int i = 0; i < MAX_ITEMEFFECTS; i++)
	{
		if (mItemEffect[i] && mItemEffectOwnerNode[i])
		{
			SceneNode* tItemEffectNode = mSceneMgr->getSceneNode("ItemFXNode" + StringConverter::toString(i));
			if (!tItemEffectNode)
			{
				printf("WARNING: Item effect node %d not found!\n", i);
				continue;
			}

			// ОТЛАДОЧНЫЙ ВЫВОД (можно убрать потом)
			static int debugCounter = 0;
			if (debugCounter++ % 100 == 0 && itemEffectBone[i] != "")
			{
				printf("Updating item effect %d, bone: %s\n", i, itemEffectBone[i].c_str());
			}

			// ЕСЛИ эффект привязан к кости - обновляем через кость
			if (itemEffectBone[i] != "")
			{
				Entity* targetEntity = 0;
				SceneNode::ObjectIterator it = mItemEffectOwnerNode[i]->getAttachedObjectIterator();
				while (it.hasMoreElements())
				{
					MovableObject* obj = it.getNext();
					if (obj->getMovableType() == "Entity")
					{
						Entity* ent = static_cast<Entity*>(obj);
						if (ent->getSkeleton() && ent->getSkeleton()->hasBone(itemEffectBone[i]))
						{
							targetEntity = ent;
							break;
						}
					}
				}

				if (targetEntity && targetEntity->getSkeleton())
				{
					try
					{
						Bone* tBone = targetEntity->getSkeleton()->getBone(itemEffectBone[i]);
						if (tBone)
						{
							// ОБНОВЛЯЕМ мировую позицию эффекта относительно кости
							tItemEffectNode->setPosition(
								targetEntity->getParentSceneNode()->_getDerivedPosition() +
								tBone->_getDerivedPosition() +
								tBone->_getDerivedOrientation() * itemEffectOffset[i]
								);
							tItemEffectNode->setOrientation(tBone->_getDerivedOrientation());

							// ОТЛАДКА позиций
							if (debugCounter % 100 == 0)
							{
								printf("Effect %d at: (%.2f, %.2f, %.2f), Bone: %s at: (%.2f, %.2f, %.2f)\n",
									i,
									tItemEffectNode->getPosition().x, tItemEffectNode->getPosition().y, tItemEffectNode->getPosition().z,
									itemEffectBone[i].c_str(),
									tBone->_getDerivedPosition().x, tBone->_getDerivedPosition().y, tBone->_getDerivedPosition().z
									);
							}
						}
					}
					catch (Ogre::Exception& e)
					{
						printf("ERROR updating bone %s for effect %d: %s\n", itemEffectBone[i].c_str(), i, e.getDescription().c_str());
						// Fallback: простая позиция
						tItemEffectNode->setPosition(mItemEffectOwnerNode[i]->_getDerivedPosition() + itemEffectOffset[i]);
					}
				}
				else
				{
					// Fallback: простая позиция
					tItemEffectNode->setPosition(mItemEffectOwnerNode[i]->_getDerivedPosition() + itemEffectOffset[i]);
				}
			}
			// ИНАЧЕ (эффект без кости) - просто следуем за узлом
			else
			{
				tItemEffectNode->setPosition(mItemEffectOwnerNode[i]->_getDerivedPosition() + itemEffectOffset[i]);
				tItemEffectNode->setOrientation(mItemEffectOwnerNode[i]->_getDerivedOrientation());
			}
		}
	}
}
void MagixEffectsManager::createRibbonTrail(Entity *ent, const String &boneName, const String &trailMat, const Real &trailWidth, const ColourValue &trailColour, const ColourValue &colourChange, const String &headMat)
{
	short tID = -1;
	for (int i = 0; i<MAX_RIBBONTRAILS; i++)
	{
		if (mTrail[i] == 0)
		{
			tID = i;
			break;
		}
	}
	if (tID == -1)
	{
		tID = 0;
		destroyRibbonTrail(tID);
	}

	mTrailHead[tID] = mSceneMgr->createBillboardSet("FXTrailHead" + StringConverter::toString(tID));
	mTrailHead[tID]->setMaterialName(headMat == "" ? "Sky/StarMat" : headMat);
	mTrailHead[tID]->setDefaultDimensions(trailWidth, trailWidth);
	mTrailHead[tID]->createBillboard(0, 0, 0);
	mTrailHead[tID]->setQueryFlags(EFFECT_MASK);
	mTrailHeadEnt[tID] = ent;
	TagPoint *tTP = mTrailHeadEnt[tID]->attachObjectToBone(boneName, mTrailHead[tID]);
	if (headMat == "")mTrailHead[tID]->setVisible(false);

	mTrail[tID] = mSceneMgr->createRibbonTrail("FXTrail" + StringConverter::toString(tID));
	mTrail[tID]->setVisible(true);
	mTrail[tID]->setMaterialName(trailMat);
	mTrail[tID]->setTrailLength(50);
	mTrail[tID]->setMaxChainElements(50);
	mTrail[tID]->setInitialColour(0, trailColour);
	mTrail[tID]->setColourChange(0, colourChange);
	mTrail[tID]->setInitialWidth(0, trailWidth);
	mTrail[tID]->setWidthChange(0, 7);
	mTrail[tID]->addNode((Node*)tTP);
	mTrail[tID]->setQueryFlags(EFFECT_MASK);
	mSceneMgr->getRootSceneNode()->attachObject(mTrail[tID]);
}
void MagixEffectsManager::destroyRibbonTrail(unsigned short tID)
{
	if (mTrailHead[tID] && mSceneMgr->hasBillboardSet("FXTrailHead" + StringConverter::toString(tID)))
	{
		mTrailHeadEnt[tID]->detachObjectFromBone(mTrailHead[tID]);
		mSceneMgr->destroyBillboardSet(mTrailHead[tID]);
		mTrailHead[tID] = 0;
		mTrailHeadEnt[tID] = 0;
	}
	if (mTrail[tID] && mSceneMgr->hasRibbonTrail("FXTrail" + StringConverter::toString(tID)))
	{
		mSceneMgr->getRootSceneNode()->detachObject(mTrail[tID]);
		mSceneMgr->destroyRibbonTrail(mTrail[tID]);
		mTrail[tID] = 0;
	}
}
ParticleSystem* MagixEffectsManager::createParticle(SceneNode *node, const String &name, const Real &duration, bool unresettable, bool attachToBone, bool hasOwnNode, const Vector3 &position, const String &ownName)
{
	short tID = -1;
	for (int i = 0; i<MAX_PARTICLES; i++)
	{
		if (mParticle[i] == 0)
		{
			tID = i;
			break;
		}
	}
	if (tID == -1)
	{
		tID = 0;
		destroyParticle(tID);
	}

	mParticle[tID] = mSceneMgr->createParticleSystem(ownName == "" ? "FXParticle" + StringConverter::toString(tID) : ownName, name);
	mParticleNode[tID] = (hasOwnNode ? mSceneMgr->getRootSceneNode()->createChildSceneNode("FXParticleNode" + StringConverter::toString(tID)) : node);
	if (hasOwnNode)mParticleNode[tID]->setPosition(position);
	if (!attachToBone)node->attachObject(mParticle[tID]);
	particleDuration[tID] = duration;
	particleUnresettable[tID] = unresettable;
	particleHasOwnNode[tID] = hasOwnNode;
	particleType[tID] = name;

	return mParticle[tID];
}
void MagixEffectsManager::destroyParticle(unsigned short tID)
{
	if (mParticle[tID]/* && mSceneMgr->hasParticleSystem("FXParticle"+StringConverter::toString(tID))*/)
	{
		mParticle[tID]->detachFromParent();
		mSceneMgr->destroyParticleSystem(mParticle[tID]);
		mParticle[tID] = 0;
		if (particleHasOwnNode[tID])mSceneMgr->destroySceneNode(mParticleNode[tID]);
		mParticleNode[tID] = 0;
		particleDuration[tID] = 0;
		particleUnresettable[tID] = false;
		particleHasOwnNode[tID] = false;
		particleType[tID] = "";
	}
}
void MagixEffectsManager::destroyParticle(ParticleSystem *pSys)
{
	for (int i = 0; i<MAX_PARTICLES; i++)
	{
		if (mParticle[i] == pSys){ destroyParticle(i); return; }
	}
}
void MagixEffectsManager::destroyParticle(const String &name)
{
	for (int i = 0; i<MAX_PARTICLES; i++)
	{
		if (mParticle[i] && mParticle[i]->getName() == name){ destroyParticle(i); return; }
	}
}
ParticleSystem* MagixEffectsManager::getParticle(const String &name)
{
	for (int i = 0; i<MAX_PARTICLES; i++)
	{
		if (mParticle[i] && mParticle[i]->getName() == name){ return mParticle[i]; }
	}
	return 0;
}
void MagixEffectsManager::destroyRibbonTrailByEntity(Entity *ent)
{
	for (int i = 0; i<MAX_RIBBONTRAILS; i++)
	{
		if (mTrailHeadEnt[i] == ent)destroyRibbonTrail(i);
	}
}
void MagixEffectsManager::destroyParticleByObjectNode(SceneNode *node, const String &type)
{
	for (int i = 0; i<MAX_PARTICLES; i++)
	{
		if (mParticleNode[i] == node && (type == "" || particleType[i] == type))destroyParticle(i);
	}
}
void MagixEffectsManager::stopParticleByObjectNode(SceneNode *node, const String &type)
{
	for (int i = 0; i<MAX_PARTICLES; i++)
	{
		if (mParticleNode[i] == node && (type == "" || particleType[i] == type))
		{
			mParticle[i]->getEmitter(0)->setEmissionRate(0);
			particleDuration[i] = 0;
		}
	}
}
void MagixEffectsManager::createWaterRipple(SceneNode *owner, const Real &waterHeight, const Real &scale)
{
	short tID = -1;
	for (int i = 0; i<MAX_WATERRIPPLES; i++)
	{
		if (mWaterRipple[i] == 0)
		{
			tID = i;
			break;
		}
	}
	if (tID == -1)
	{
		tID = 0;
		destroyWaterRipple(tID);
	}

	mWaterRipple[tID] = mSceneMgr->createParticleSystem("FXWaterRipple" + StringConverter::toString(tID), "WaterRipple");
	mWaterRipple[tID]->setDefaultDimensions(scale, scale);
	mWaterRippleNode[tID] = mSceneMgr->getRootSceneNode()->createChildSceneNode("WaterRippleNode" + StringConverter::toString(tID));
	mWaterRippleNode[tID]->attachObject(mWaterRipple[tID]);
	mWaterRippleOwner[tID] = owner;
	mWaterRippleNode[tID]->setPosition(owner->getPosition().x, waterHeight, owner->getPosition().z);
}
void MagixEffectsManager::destroyWaterRipple(unsigned short tID)
{
	if (mWaterRipple[tID] && mSceneMgr->hasParticleSystem("FXWaterRipple" + StringConverter::toString(tID)))
	{
		mWaterRippleNode[tID]->detachObject(mWaterRipple[tID]);
		mSceneMgr->destroyParticleSystem(mWaterRipple[tID]);
		mSceneMgr->destroySceneNode("WaterRippleNode" + StringConverter::toString(tID));
		mWaterRipple[tID] = 0;
		mWaterRippleNode[tID] = 0;
		mWaterRippleOwner[tID] = 0;
	}
}
void MagixEffectsManager::destroyWaterRippleByOwner(SceneNode *node)
{
	for (int i = 0; i<MAX_WATERRIPPLES; i++)
	{
		if (mWaterRippleOwner[i] == node)destroyWaterRipple(i);
	}
}
void MagixEffectsManager::setWaterRippleEmissionByOwner(SceneNode *node, const Real &rate)
{
	for (int i = 0; i<MAX_WATERRIPPLES; i++)
	{
		if (mWaterRipple[i] && mWaterRippleOwner[i] == node)
		{
			mWaterRipple[i]->getEmitter(0)->setEmissionRate(rate);
			return;
		}
	}
}
void MagixEffectsManager::setWaterRippleHeightByOwner(SceneNode *node, const Real &height)
{
	for (int i = 0; i<MAX_WATERRIPPLES; i++)
	{
		if (mWaterRipple[i] && mWaterRippleOwner[i] == node)
		{
			mWaterRippleNode[i]->setPosition(node->getPosition().x, height, node->getPosition().z);
			return;
		}
	}
}
bool MagixEffectsManager::hasWaterRippleOwner(SceneNode *node)
{
	for (int i = 0; i<MAX_WATERRIPPLES; i++)
	{
		if (mWaterRippleOwner[i] == node)return true;
	}
	return false;
}
void MagixEffectsManager::createBreather(Entity *owner, const String &type, const Vector3 &offset, const Real &scale)
{
	short tID = -1;
	for (int i = 0; i<MAX_BREATHERS; i++)
	{
		if (mBreather[i] == 0)
		{
			tID = i;
			break;
		}
	}
	if (tID == -1)
	{
		tID = 0;
		destroyBreather(tID);
	}

	mBreather[tID] = mSceneMgr->createParticleSystem("FXBreather" + StringConverter::toString(tID), type);
	mBreather[tID]->setDefaultDimensions(scale, scale);
	//TagPoint *tTP = owner->attachObjectToBone("Mouth",mBreather[tID],Quaternion::IDENTITY,offset);
	SceneNode* tBreatherNode = owner->getParentSceneNode()->createChildSceneNode("BreatherNode" + StringConverter::toString(tID));
	tBreatherNode->attachObject(mBreather[tID]);
	mBreatherOwner[tID] = owner;
	breatherType[tID] = type;
	breatherOffset[tID] = offset;
}
void MagixEffectsManager::destroyBreather(unsigned short tID)
{
	if (mBreather[tID] && mSceneMgr->hasParticleSystem("FXBreather" + StringConverter::toString(tID)))
	{
		SceneNode* tBreatherNode = mSceneMgr->getSceneNode("BreatherNode" + StringConverter::toString(tID));
		if (!tBreatherNode)return;
		//mBreatherOwner[tID]->detachObjectFromBone(mBreather[tID]);
		mSceneMgr->destroyParticleSystem(mBreather[tID]);
		mSceneMgr->destroySceneNode("BreatherNode" + StringConverter::toString(tID));
		mBreather[tID] = 0;
		mBreatherOwner[tID] = 0;
		breatherType[tID] = "";
		breatherOffset[tID] = Vector3::ZERO;
	}
}
void MagixEffectsManager::destroyBreatherByOwner(Entity *ent, const String &type)
{
	for (int i = 0; i<MAX_BREATHERS; i++)
	{
		if (mBreatherOwner[i] == ent && (type == "" || breatherType[i] == type))destroyBreather(i);
	}
}
void MagixEffectsManager::setBreatherEmissionByOwner(Entity *ent, const Real &rate, const String &type)
{
	for (int i = 0; i<MAX_BREATHERS; i++)
	{
		if (mBreather[i] && mBreatherOwner[i] == ent && (type == "" || breatherType[i] == type))
		{
			mBreather[i]->getEmitter(0)->setEmissionRate(rate);
		}
	}
}
bool MagixEffectsManager::hasBreatherOwner(Entity *ent, const String &type)
{
	for (int i = 0; i<MAX_BREATHERS; i++)
	{
		if (mBreatherOwner[i] == ent && (type == "" || breatherType[i] == type))return true;
	}
	return false;
}
void MagixEffectsManager::createItemParticle(SceneNode *node, const String &particleType)
{
	short tID = -1;
	for (int i = 0; i<MAX_ITEMSPARKLE; i++)
	{
		if (mItemParticle[i] == 0)
		{
			tID = i;
			break;
		}
	}
	if (tID == -1)
	{
		tID = 0;
		destroyItemParticle(tID);
	}

	mItemParticle[tID] = mSceneMgr->createParticleSystem("ItemParticle" + StringConverter::toString(tID), particleType);
	mItemParticleNode[tID] = node;
	node->attachObject(mItemParticle[tID]);
}
void MagixEffectsManager::destroyItemParticle(unsigned short tID)
{
	if (mItemParticle[tID] && mSceneMgr->hasParticleSystem("ItemParticle" + StringConverter::toString(tID)))
	{
		mItemParticleNode[tID]->detachObject(mItemParticle[tID]);
		mSceneMgr->destroyParticleSystem(mItemParticle[tID]);
		mItemParticle[tID] = 0;
		mItemParticleNode[tID] = 0;
	}
}
void MagixEffectsManager::destroyItemParticleByObjectNode(SceneNode *node)
{
	for (int i = 0; i<MAX_ITEMSPARKLE; i++)
	{
		if (mItemParticleNode[i] == node)destroyItemParticle(i);
	}
}
void MagixEffectsManager::createItemEffect(SceneNode *ownerNode, const String &type, const String &bone, const Vector3 &offset, const Real &scale, const String &uniqueID)
{
	printf("=== CREATE ITEM EFFECT START ===\n");
	printf("OwnerNode: %p, Type: %s, Bone: %s, UniqueID: %s\n",
		ownerNode, type.c_str(), bone.c_str(), uniqueID.c_str());

	if (!ownerNode)
	{
		printf("ERROR: createItemEffect called with null ownerNode!\n");
		return;
	}

	if (type == "")
	{
		printf("ERROR: createItemEffect called with empty type!\n");
		return;
	}

	short tID = -1;
	for (int i = 0; i < MAX_ITEMEFFECTS; i++)
	{
		if (mItemEffect[i] == 0)
		{
			tID = i;
			break;
		}
	}
	if (tID == -1)
	{
		tID = 0;
		destroyItemEffect(tID);
	}

	printf("Creating item effect with ID: %d\n", tID);

	try
	{
		// Создаем партикл систему
		String particleName = "ItemFX" + StringConverter::toString(tID);
		if (uniqueID != "")
		{
			particleName = uniqueID;
		}

		if (mSceneMgr->hasParticleSystem(particleName))
		{
			printf("WARNING: ParticleSystem %s already exists! Destroying...\n", particleName.c_str());
			ParticleSystem* oldPS = mSceneMgr->getParticleSystem(particleName);
			mSceneMgr->destroyParticleSystem(oldPS);
		}

		mItemEffect[tID] = mSceneMgr->createParticleSystem(particleName, type);
		if (!mItemEffect[tID])
		{
			printf("ERROR: Failed to create particle system %s\n", particleName.c_str());
			return;
		}

		mItemEffect[tID]->setDefaultDimensions(mItemEffect[tID]->getDefaultWidth() * scale, mItemEffect[tID]->getDefaultHeight() * scale);

		// Сохраняем уникальный идентификатор
		itemEffectUniqueID[tID] = uniqueID;

		// ВСЕГДА создаем независимый нод
		SceneNode* tItemEffectNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("ItemFXNode" + StringConverter::toString(tID));
		tItemEffectNode->attachObject(mItemEffect[tID]);

		// ПРОСТАЯ ПОЗИЦИЯ - будем обновлять в update методе
		tItemEffectNode->setPosition(ownerNode->_getDerivedPosition() + offset);
		tItemEffectNode->setScale(scale, scale, scale);

		mItemEffectOwnerNode[tID] = ownerNode;
		mItemEffectOwner[tID] = 0;

		// Сохраняем остальные параметры
		itemEffectBone[tID] = bone;
		itemEffectType[tID] = type;
		itemEffectOffset[tID] = offset;

		printf("SUCCESS: Created item effect %d, will update position in update()\n", tID);
	}
	catch (Ogre::Exception& e)
	{
		printf("ERROR in createItemEffect: %s\n", e.getFullDescription().c_str());
		if (mItemEffect[tID])
		{
			mSceneMgr->destroyParticleSystem(mItemEffect[tID]);
			mItemEffect[tID] = 0;
		}
	}
	printf("=== CREATE ITEM EFFECT END ===\n");
}

void MagixEffectsManager::destroyItemEffectByOwnerNode(SceneNode *node, const String &type)
{
	printf("=== DESTROY ITEM EFFECT BY OWNER NODE START ===\n");
	printf("Looking for effects with owner node %p, type: %s\n", node, type.c_str());

	for (int i = 0; i < MAX_ITEMEFFECTS; i++)
	{
		bool shouldDestroy = false;
		String reason = "";

		// Если эффект прикреплен к узлу напрямую
		if (mItemEffectOwnerNode[i] == node && (type == "" || itemEffectType[i] == type))
		{
			shouldDestroy = true;
			reason = "Direct node owner match";
		}
		// Если эффект прикреплен к ентити, которое принадлежит этому узлу
		else if (mItemEffectOwner[i] && mItemEffectOwner[i]->getParentSceneNode() == node && (type == "" || itemEffectType[i] == type))
		{
			shouldDestroy = true;
			reason = "Entity owner match";
		}

		if (shouldDestroy)
		{
			printf("Destroying effect %d: uniqueID='%s', type='%s', bone='%s' - Reason: %s\n",
				i, itemEffectUniqueID[i].c_str(), itemEffectType[i].c_str(), itemEffectBone[i].c_str(), reason.c_str());
			destroyItemEffect(i);
		}
	}
	printf("=== DESTROY ITEM EFFECT BY OWNER NODE END ===\n");
}
void MagixEffectsManager::destroyItemEffect(unsigned short tID)
{
	printf("Destroying item effect %d\n", tID);

	String particleName = "ItemFX" + StringConverter::toString(tID);
	String nodeName = "ItemFXNode" + StringConverter::toString(tID);

	// Уничтожаем партикл систему
	if (mItemEffect[tID])
	{
		try
		{
			// Если прикреплен к кости - открепляем от ентити
			if (mItemEffectOwner[tID] && itemEffectBone[tID] != "")
			{
				// ПРАВИЛЬНЫЙ СПОСОБ: открепляем объект от кости
				mItemEffectOwner[tID]->detachObjectFromBone(mItemEffect[tID]);
				printf("Detached particle system from bone: %s\n", itemEffectBone[tID].c_str());
			}
			// Если прикреплен к узлу - открепляем от узла
			else if (mSceneMgr->hasParticleSystem(particleName))
			{
				ParticleSystem* ps = mSceneMgr->getParticleSystem(particleName);
				ps->detachFromParent();
				printf("Detached particle system from node\n");
			}

			mSceneMgr->destroyParticleSystem(mItemEffect[tID]);
			printf("Destroyed particle system: %s\n", particleName.c_str());
		}
		catch (Ogre::Exception& e)
		{
			printf("ERROR destroying particle system %s: %s\n", particleName.c_str(), e.getDescription().c_str());
		}
	}

	// Уничтожаем узел (только для независимых эффектов)
	if (mSceneMgr->hasSceneNode(nodeName))
	{
		try
		{
			SceneNode* node = mSceneMgr->getSceneNode(nodeName);
			node->detachAllObjects();
			mSceneMgr->destroySceneNode(node);
			printf("Destroyed scene node: %s\n", nodeName.c_str());
		}
		catch (Ogre::Exception& e)
		{
			printf("ERROR destroying scene node %s: %s\n", nodeName.c_str(), e.getDescription().c_str());
		}
	}

	// Сбрасываем переменные
	mItemEffect[tID] = 0;
	mItemEffectOwner[tID] = 0;
	mItemEffectOwnerNode[tID] = 0;
	itemEffectBone[tID] = "";
	itemEffectType[tID] = "";
	itemEffectOffset[tID] = Vector3::ZERO;

	printf("SUCCESS: Completely destroyed item effect %d\n", tID);
}
void MagixEffectsManager::destroyItemEffectByUniqueID(const String &uniqueID)
{
	printf("=== DESTROY ITEM EFFECT BY UNIQUE ID START ===\n");
	printf("Looking for effect with unique ID: %s\n", uniqueID.c_str());

	for (int i = 0; i < MAX_ITEMEFFECTS; i++)
	{
		printf("Effect slot %d: ", i);
		if (mItemEffect[i])
		{
			printf("EXISTS, uniqueID='%s', type='%s', bone='%s'",
				itemEffectUniqueID[i].c_str(),
				itemEffectType[i].c_str(),
				itemEffectBone[i].c_str());

			if (itemEffectUniqueID[i] == uniqueID)
			{
				printf(" <- MATCH FOUND! Destroying...\n");
				destroyItemEffect(i);
				printf("=== DESTROY ITEM EFFECT BY UNIQUE ID END (SUCCESS) ===\n");
				return;
			}
			else
			{
				printf(" <- NO MATCH\n");
			}
		}
		else
		{
			printf("EMPTY\n");
		}
	}
	printf("=== DESTROY ITEM EFFECT BY UNIQUE ID END (NOT FOUND) ===\n");
}