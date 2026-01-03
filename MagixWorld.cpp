#include "MagixWorld.h"
#include "HeightFunction.h"
#include "MagixConst.h"
#include "MagixCritter.h"
#include "MagixCritterManager.h"
#include <algorithm> 

MagixWorld::MagixWorld()
{
	mMainCamera = 0;
	mRayQuery = 0;
	// === КРАФТ СИСТЕМА === //
	// Инициализация рецептов крафта
	initializeCraftRecipes();
	// === КОНЕЦ КРАФТ СИСТЕМЫ === //

	mDef = 0;
	mGameStateManager = 0;
	mGameStateManager = 0;
	mSoundManager = 0;
	mCollisionManager = 0;
	mSkyManager = 0;
	mSceneMgr = 0;
	mWindow = 0;
	mTrees = 0;
	mGrass = 0;
	mBushes = 0;
	mLargeTrees = 0;
	mFloatingBushes = 0;
	numTreeMeshes = 0;
	numBushMeshes = 0;
	mCeiling = 0;
	mCeilingNode = 0;
	for (int i = 0; i<MAX_LIGHTS; i++)mLight[i] = 0;
	worldName = "";
	worldSize = Vector2::ZERO;
	worldBounds = Vector2::ZERO;
	spawnSquare = Vector3::ZERO;
	numWaterPlanes = 0;
	numObjects = 0;
	numParticles = 0;
	numLights = 0;
	numBillboards = 0;
	ceilingHeight = 0;
	isInterior = false;
	mapEffect = "";
	staticGeometryX = 0;
	staticGeometryY = 0;
	critterSpawnList.clear();
	critterRoamAreaList.clear();
	critterSpawnLimit = 0;
	isArena = false;
}

MagixWorld::~MagixWorld()
{
	unloadWorld();
	critterSpawnList.clear();
	critterRoamAreaList.clear();

	if (mReflectRtt)
	{
		mReflectRtt->removeAllViewports();
		mReflectRtt = 0;
	}
	if (!mReflectTex.isNull())
	{
		mReflectTex.setNull();
	}

	if (mRayQuery) delete mRayQuery;
}

void MagixWorld::initialize(SceneManager* sceneMgr, RenderWindow* window, MagixExternalDefinitions* def,
	MagixGameStateManager* gameStateMgr, MagixSoundManager* soundMgr,
	MagixCollisionManager* collisionMgr, MagixSkyManager* skyMgr,
	MagixCamera* mainCam, MagixCritterManager* critterMgr)
{
	mSceneMgr = sceneMgr;
	mWindow = window;
	mDef = def;
	mGameStateManager = gameStateMgr;
	mSoundManager = soundMgr;
	mCollisionManager = collisionMgr;
	mSkyManager = skyMgr;
	mMainCamera = mainCam;
	mCritterManager = critterMgr;

	// Создаем ray query для определения позиции воды
	mRayQuery = sceneMgr->createRayQuery(Ray());

	initializeCustomMeshes();
	mCeiling = sceneMgr->createEntity("Ceiling", "Ceiling.mesh");
	mCeiling->setCastShadows(false);
	mCeilingNode = sceneMgr->getRootSceneNode()->createChildSceneNode();
	mCeilingNode->attachObject(mCeiling);
	mCeilingNode->setVisible(false);
}
void MagixWorld::initializeCustomMeshes()
{
	Plane waterPlane;
	waterPlane.d = 0;
	waterPlane.normal = Vector3::UNIT_Y;

	MeshManager::getSingleton().createPlane("WaterPlaneMesh",
		ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		waterPlane,
		500, 500,    // размер плоскости
		1, 1,        // сегменты
		true,        // нормализованные UV
		1,
		1, 1,        // ИСПРАВЛЕНО: UV масштаб 1x1 (было 2,2)
		Vector3::UNIT_Z);
}

void MagixWorld::reset()
{
	const String tWorldName = worldName;
	unloadWorld();
	loadWorld(tWorldName);
}

void MagixWorld::update()
{
	if (mGrass) mGrass->update();
	if (mTrees) mTrees->update();
	if (mBushes) mBushes->update();
	if (mLargeTrees) mLargeTrees->update();
	if (mFloatingBushes) mFloatingBushes->update();

	// УБРАНО: updateUnderWater();
	// УБРАНО: updateCameraReflector();
	// УБРАНО: весь блок с отражениями

	const Vector2 camPos = (mSceneMgr->getCamera("PlayerCam")->isAttached() ?
		Vector2(mSceneMgr->getCamera("PlayerCam")->getDerivedPosition().x, mSceneMgr->getCamera("PlayerCam")->getDerivedPosition().z) :
		Vector2(mSceneMgr->getCamera("PlayerCam")->getPosition().x, mSceneMgr->getCamera("PlayerCam")->getPosition().z));
	const Real FARCLIP_SQUARED = Math::Sqr(mDef->viewDistance * 2500 + 500);
	for (int i = 1; i <= numObjects; i++)
	{
		Entity* ent = mSceneMgr->getEntity("WorldObject" + StringConverter::toString(i));
		if (Vector2(camPos.x - ent->getParentSceneNode()->getPosition().x, camPos.y - ent->getParentSceneNode()->getPosition().z).squaredLength()>FARCLIP_SQUARED)
		{
			if (ent->isVisible())ent->setVisible(false);
		}
		else
		{
			if (!ent->isVisible())ent->setVisible(true);
		}
	}
	for (int i = 1; i <= numParticles; i++)
	{
		ParticleSystem* pSys = mSceneMgr->getParticleSystem("WorldParticle" + StringConverter::toString(i));
		if (Vector2(camPos.x - pSys->getParentSceneNode()->getPosition().x, camPos.y - pSys->getParentSceneNode()->getPosition().z).squaredLength()>FARCLIP_SQUARED)
		{
			if (pSys->isVisible())pSys->setVisible(false);
		}
		else
		{
			if (!pSys->isVisible())pSys->setVisible(true);
		}
	}
	for (int i = 1; i <= numBillboards; i++)
	{
		BillboardSet* set = mSceneMgr->getBillboardSet("WorldBillboard" + StringConverter::toString(i));
		if (Vector2(camPos.x - set->getParentSceneNode()->getPosition().x, camPos.y - set->getParentSceneNode()->getPosition().z).squaredLength()>FARCLIP_SQUARED)
		{
			if (set->isVisible())set->setVisible(false);
		}
		else
		{
			if (!set->isVisible())set->setVisible(true);
		}
	}
	for (int i = 0; i<MAX_LIGHTS; i++)
	{
		if (mLight[i])
		{
			if (Vector2(camPos.x - mLight[i]->getPosition().x, camPos.y - mLight[i]->getPosition().z).squaredLength()>810000)
			{
				if (mLight[i]->isVisible())mLight[i]->setVisible(false);
			}
			else
			{
				if (!mLight[i]->isVisible())mLight[i]->setVisible(true);
			}
		}
	}
}
// Добавьте эти функции в начало MagixWorld.cpp, перед loadWorld

void MagixWorld::clearOldImpostors()
{
#ifdef _WIN32
	// Удаляем старые импостеры из корневой папки
	WIN32_FIND_DATA findFileData;
	HANDLE hFind;

	// Ищем файлы Impostor.*.png
	hFind = FindFirstFile("Impostor.*.png", &findFileData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do {
			String filename = findFileData.cFileName;
			// Удаляем файлы импостеров
			DeleteFile(filename.c_str());
		} while (FindNextFile(hFind, &findFileData) != 0);
		FindClose(hFind);
	}

	// Также ищем файлы типа Impostor.*.*.png
	hFind = FindFirstFile("Impostor.*.*.png", &findFileData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do {
			String filename = findFileData.cFileName;
			DeleteFile(filename.c_str());
		} while (FindNextFile(hFind, &findFileData) != 0);
		FindClose(hFind);
	}
#endif
}

void MagixWorld::moveImpostorsToFolder()
{
#ifdef _WIN32
	// Если нет имени мира, ничего не делаем
	if (worldName.empty())
		return;

	// Создаем папку для импостеров текущего мира
	String folderName = "impostors_" + worldName;
	CreateDirectory(folderName.c_str(), NULL);

	WIN32_FIND_DATA findFileData;
	HANDLE hFind;

	// Перемещаем ВСЕ файлы импостеров в папку
	hFind = FindFirstFile("Impostor.*", &findFileData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do {
			String oldName = findFileData.cFileName;
			String newName = folderName + "/" + oldName;

			// Перемещаем файл
			MoveFile(oldName.c_str(), newName.c_str());

		} while (FindNextFile(hFind, &findFileData) != 0);
		FindClose(hFind);
	}
#endif
}

void MagixWorld::loadWorld(const String &name)
{
	String tTerrain = "", tGrassMat = "", tGrassMap = "", tGrassColourMap = "", tTree1 = "", tTree2 = "", tTree3 = "", tBush1 = "", tBush2 = "", tBush3 = "";
	unsigned short tTreeCount;
	vector<const std::pair<String, bool>>::type tTreesFilename, tBushesFilename, tFloatingBushesFilename;
	String tCritterSpawnFilename = "";
	tTreesFilename.clear();
	tBushesFilename.clear();
	tFloatingBushesFilename.clear();

	worldBounds = Vector2(50, 50);
	if (!mDef->loadWorld("media/terrains/" + name + "/" + name + ".world", tTerrain, worldSize.x, worldSize.y, spawnSquare, worldBounds))return;

	if (worldName != "")unloadWorld();
	worldName = name;

	Camera *camera = mSceneMgr->getCamera("PlayerCam");
	if (mDef->pagedGeometryForced)
	{
		mDef->pagedGeometryForced = false;
		mDef->pagedGeometryOn = false;
	}
	mSceneMgr->setWorldGeometry(tTerrain);
	mSkyManager->setWeatherCycle("Rain");
	mSkyManager->setInteriorSky(false);
	mSkyManager->freezeDayTime(false);
	critterSpawnList.clear();
	critterRoamAreaList.clear();
	critterSpawnLimit = 0;

	if (mGameStateManager->getGameState() > GAMESTATE_CHARSCREEN&&!mSoundManager->getRandomPlaylist())
	{
		mSoundManager->stopMusic();
		mSoundManager->setRandomPlaylist(true);
	}

	//load misc objects
	MovableObject::setDefaultQueryFlags(WORLDOBJECT_MASK);
	const String tBuffer = mDef->loadWorldObjects("media/terrains/" + worldName + "/" + worldName + ".world") +
		mDef->loadWorldObjects("media/terrains/" + worldName + "/CustomPortals.txt");
	if (tBuffer.length() > 0)
	{
		const vector<String>::type tPart = StringUtil::split(tBuffer, "[#");
		for (int i = 0; i < int(tPart.size()); i++)
		{
			const vector<String>::type tLine = StringUtil::split(tPart[i], "\n");
			if (tLine.size() > 0)
			{
				if (StringUtil::startsWith(tLine[0], "Portal", false) && tLine.size() == 4)
				{
					const Vector3 tPosition = StringConverter::parseVector3(tLine[1]);
					const Real tRange = StringConverter::parseReal(tLine[2]);
					const String tDestName = tLine[3];
					mCollisionManager->createPortal(tPosition, tRange, tDestName);
				}
				else if (StringUtil::startsWith(tLine[0], "Gate", false) && tLine.size() >= 5)
				{
					const String tMatName = tLine[1];
					const Vector3 tPosition = StringConverter::parseVector3(tLine[2]);
					const String tDestName = tLine[3];
					const vector<String>::type tCoords = StringUtil::split(tLine[4], " ", 3);
					Vector3 tDestVect;
					bool tHasVectY = true;
					if (tCoords.size() == 2)
					{
						tDestVect.x = StringConverter::parseReal(tCoords[0]);
						tDestVect.z = StringConverter::parseReal(tCoords[1]);
						tHasVectY = false;
					}
					else tDestVect = StringConverter::parseVector3(tLine[4]);
					mCollisionManager->createGate(tPosition, tDestName, tDestVect, tMatName, tHasVectY);
				}
				else if (StringUtil::startsWith(tLine[0], "WaterPlane", false) && tLine.size() >= 4)
				{
					const Vector3 tPosition = StringConverter::parseVector3(tLine[1]);
					const Real tScaleX = StringConverter::parseReal(tLine[2]);
					const Real tScaleZ = StringConverter::parseReal(tLine[3]);
					String tMat = "Terrain/CalmWater";
					String tSound = "";
					bool tIsSolid = false;
					if (tLine.size() > 4)tMat = tLine[4];
					if (tLine.size() > 5)tSound = tLine[5];
					if (tLine.size() > 6)tIsSolid = StringConverter::parseBool(tLine[6]);
					addWaterPlane(tPosition, tScaleX, tScaleZ, tMat, tSound);
					mCollisionManager->createWaterBox(tPosition, tScaleX * 500, tScaleZ * 500, tIsSolid);
				}
				else if (StringUtil::startsWith(tLine[0], "Object", false) && tLine.size() >= 5)
				{
					const Vector3 tPosition = StringConverter::parseVector3(tLine[2]);
					const Vector3 tScale = StringConverter::parseVector3(tLine[3]);
					const Vector3 tRotation = StringConverter::parseVector3(tLine[4]);
					String tSound = "";
					if (tLine.size() > 5)tSound = tLine[5];
					String tMatName = "";
					if (tLine.size() > 6)tMatName = tLine[6];

					// СОЗДАЕМ ОБЪЕКТ
					addObject(tLine[1], tPosition, tScale, tRotation.x, tRotation.y, tRotation.z, tSound, tMatName);

					// === КРАФТ СИСТЕМА === //
					// ПРОВЕРЯЕМ ТЕГ ДЛЯ СТАНЦИИ КРАФТА
					if (tLine.size() > 7 && tLine[7] == "crafting_station")
					{
						// Получаем ноду последнего созданного объекта
						SceneNode* node = mSceneMgr->getEntity("WorldObject" + StringConverter::toString(numObjects))->getParentSceneNode();
						// Добавляем в список станций крафта
						craftingStations.push_back(node);
					}
					// === КОНЕЦ КРАФТ СИСТЕМЫ === //
				}
				else if (StringUtil::startsWith(tLine[0], "Particle", false) && tLine.size() >= 3)
				{
					const Vector3 tPosition = StringConverter::parseVector3(tLine[2]);
					String tSound = "";
					if (tLine.size() > 3)tSound = tLine[3];
					addParticle(tLine[1], tPosition, tSound);
				}
				else if (StringUtil::startsWith(tLine[0], "WeatherCycle", false) && tLine.size() == 2)
				{
					mSkyManager->setWeatherCycle(tLine[1]);
				}
				else if (StringUtil::startsWith(tLine[0], "Interior", false) && tLine.size() >= 3)
				{
					isInterior = true;
					if (tLine.size() > 3)mSkyManager->setInteriorSky(true, StringConverter::parseColourValue(tLine[3]));
					else mSkyManager->setInteriorSky(true);
					ceilingHeight = StringConverter::parseReal(tLine[1]);
					mCeiling->setMaterialName(tLine[2]);
					mCeilingNode->setPosition(worldSize.x / 2, ceilingHeight + 20, worldSize.y / 2);
					mCeilingNode->setScale(worldSize.x / 2, 200, worldSize.y / 2);
					mCeilingNode->setVisible(true);
				}
				else if (StringUtil::startsWith(tLine[0], "Light", false) && tLine.size() == 3)
				{
					const Vector3 tPosition = StringConverter::parseVector3(tLine[1]);
					const ColourValue tColour = StringConverter::parseColourValue(tLine[2]);
					addLight(tPosition, tColour);
				}
				else if (StringUtil::startsWith(tLine[0], "Billboard", false) && tLine.size() == 4)
				{
					const Vector3 tPosition = StringConverter::parseVector3(tLine[1]);
					const Vector2 tScale = StringConverter::parseVector2(tLine[2]);
					const String tMat = tLine[3];
					addBillboard(tPosition, tScale, tMat);
				}
				else if (StringUtil::startsWith(tLine[0], "SphereWall", false) && tLine.size() == 4)
				{
					const Vector3 tPosition = StringConverter::parseVector3(tLine[1]);
					const Real tRange = StringConverter::parseReal(tLine[2]);
					const bool tIsInside = StringConverter::parseBool(tLine[3]);
					mCollisionManager->createSphereWall(tPosition, tRange, tIsInside);
				}
				else if (StringUtil::startsWith(tLine[0], "BoxWall", false) && tLine.size() == 4)
				{
					const Vector3 tPosition = StringConverter::parseVector3(tLine[1]);
					const Vector2 tRange = StringConverter::parseVector2(tLine[2]);
					const bool tIsInside = StringConverter::parseBool(tLine[3]);
					mCollisionManager->createBoxWall(tPosition, Vector3(tRange.x, 0, tRange.y), tIsInside);
				}
				else if (StringUtil::startsWith(tLine[0], "MapEffect", false) && tLine.size() == 2)
				{
					mapEffect = tLine[1];
				}
				else if (StringUtil::startsWith(tLine[0], "Grass", false) && tLine.size() == 4)
				{
					tGrassMat = tLine[1];
					tGrassMap = tLine[2];
					tGrassColourMap = tLine[3];
				}
				else if (StringUtil::startsWith(tLine[0], "RandomTrees", false) && tLine.size() == 5)
				{
					tTree1 = tLine[1];
					tTree2 = tLine[2];
					tTree3 = tLine[3];
					tTreeCount = StringConverter::parseInt(tLine[4]);
					if (tTreeCount > 500 && !mDef->pagedGeometryOn)
					{
						mDef->pagedGeometryOn = true;
						mDef->pagedGeometryForced = true;
					}
				}
				else if (StringUtil::startsWith(tLine[0], "RandomBushes", false) && tLine.size() == 4)
				{
					tBush1 = tLine[1];
					tBush2 = tLine[2];
					tBush3 = tLine[3];
					if (worldSize.x > 10000 && !mDef->pagedGeometryOn)
					{
						mDef->pagedGeometryOn = true;
						mDef->pagedGeometryForced = true;
					}
				}
				else if (StringUtil::startsWith(tLine[0], "Trees", false) && tLine.size() >= 2)
				{
					for (int j = 1; j < (int)tLine.size(); j++)tTreesFilename.push_back(std::pair<String, bool>(tLine[j], false));
				}
				else if (StringUtil::startsWith(tLine[0], "Bushes", false) && tLine.size() >= 2)
				{
					for (int j = 1; j < (int)tLine.size(); j++)tBushesFilename.push_back(std::pair<String, bool>(tLine[j], false));
				}
				else if (StringUtil::startsWith(tLine[0], "FloatingBushes", false) && tLine.size() >= 2)
				{
					for (int j = 1; j < (int)tLine.size(); j++)tFloatingBushesFilename.push_back(std::pair<String, bool>(tLine[j], false));
				}
				else if (StringUtil::startsWith(tLine[0], "NewTrees", false) && tLine.size() >= 2)
				{
					for (int j = 1; j < (int)tLine.size(); j++)tTreesFilename.push_back(std::pair<String, bool>(tLine[j], true));
				}
				else if (StringUtil::startsWith(tLine[0], "NewBushes", false) && tLine.size() >= 2)
				{
					for (int j = 1; j < (int)tLine.size(); j++)tBushesFilename.push_back(std::pair<String, bool>(tLine[j], true));
				}
				else if (StringUtil::startsWith(tLine[0], "NewFloatingBushes", false) && tLine.size() >= 2)
				{
					for (int j = 1; j < (int)tLine.size(); j++)tFloatingBushesFilename.push_back(std::pair<String, bool>(tLine[j], true));
				}
				else if (StringUtil::startsWith(tLine[0], "CollBox", false) && tLine.size() == 3)
				{
					const Vector3 tPosition = StringConverter::parseVector3(tLine[1]);
					const Vector3 tRange = StringConverter::parseVector3(tLine[2]);
					mCollisionManager->createCollBox(tPosition, tRange);
				}
				else if (StringUtil::startsWith(tLine[0], "CollSphere", false) && tLine.size() == 3)
				{
					const Vector3 tPosition = StringConverter::parseVector3(tLine[1]);
					const Real tRange = StringConverter::parseReal(tLine[2]);
					mCollisionManager->createCollSphere(tPosition, tRange);
				}
				else if (StringUtil::startsWith(tLine[0], "SpawnCritters", false) && tLine.size() == 2)
				{
#if CUSTOMCRITTERS==1
					tCritterSpawnFilename = tLine[1];
#endif
				}
				else if (StringUtil::startsWith(tLine[0], "FreezeTime", false) && tLine.size() == 2)
				{
					mSkyManager->freezeDayTime(true, StringConverter::parseReal(tLine[1]));
				}
				else if (StringUtil::startsWith(tLine[0], "Music", false) && tLine.size() == 2)
				{
					mSoundManager->setRandomPlaylist(false);
					mSoundManager->playMusic(tLine[1].c_str());
				}
				else if (StringUtil::startsWith(tLine[0], "Arena", false))
				{
					isArena = StringConverter::parseBool(tLine[1]);
				}
			}
		}
	}
	// Load critter spawn list
	if (!mDef->loadCritterSpawnList(worldName, critterSpawnLimit, critterSpawnList, critterRoamAreaList, tCritterSpawnFilename))
	{
		// Non-crittered map Draw Points
		critterSpawnList.clear();
		const Real tSize = (Real)((worldSize.x + worldSize.y) * 0.5);

		// Draw Point остается как был
		critterSpawnList.push_back(WorldCritter("Draw Point", (Real)(0.0001 * tSize * 0.0001)));

		// === КРАФТ СИСТЕМА === //
		static unsigned short nextCritterID = 1000; // счётчик Critter ID

		// Проверяем, что менеджер Critter’ов существует
		if (!mCritterManager)
			return; // безопасно, если CritterManager ещё не создан

		// Crafting Station 1
		Critter craftStation1;
		craftStation1.type = "Crafting Station";
		craftStation1.mesh = "Egg";
		craftStation1.hp = 1;
		craftStation1.maxSpeed = 0;
		craftStation1.isUncustomizable = true;
		craftStation1.isCraftingStation = true;
		craftStation1.invulnerable = true;
		craftStation1.friendly = true;
		craftStation1.scale = 2.0f;

		MagixCritter* tC1 = mCritterManager->createCritter(nextCritterID++, craftStation1, Vector3::ZERO, (short)0);
		addCraftingStation(tC1->getObjectNode());

		// Crafting Station 2
		Critter craftStation2;
		craftStation2.type = "Crafting Station";
		craftStation2.mesh = "Egg";
		craftStation2.hp = 1;
		craftStation2.maxSpeed = 0;
		craftStation2.isUncustomizable = true;
		craftStation2.isCraftingStation = true;
		craftStation2.invulnerable = true;
		craftStation2.friendly = true;
		craftStation2.scale = 2.0f;

		MagixCritter* tC2 = mCritterManager->createCritter(nextCritterID++, craftStation2, Vector3::ZERO, (short)0);
		addCraftingStation(tC2->getObjectNode());
		// === КОНЕЦ КРАФТ СИСТЕМЫ === //

		// При необходимости увеличиваем лимит Critter’ов
		if (tSize <= 5000) critterSpawnLimit = 2;
		else if (tSize <= 8000) critterSpawnLimit = 3;
	}






	//if(!mDef->hasVertexProgram)return;

	HeightFunction::initialize(mSceneMgr);
	if (tGrassMat != "")
	{
		//-------------------------------------- LOAD GRASS --------------------------------------
		//Create and configure a new PagedGeometry instance for grass
		mGrass = new PagedGeometry(camera, 250);
		mGrass->addDetailLevel<GrassPage>(1500);

		//Create a GrassLoader object
		GrassLoader *grassLoader = new GrassLoader(mGrass);
		mGrass->setPageLoader(grassLoader);	//Assign the "treeLoader" to be used to load geometry for the PagedGeometry instance

		//Supply a height function to GrassLoader so it can calculate grass Y values
		//HeightFunction::initialize(mSceneMgr);
		grassLoader->setHeightFunction(&HeightFunction::getTerrainHeight);

		//Add some grass to the scene with GrassLoader::addLayer()
		GrassLayer *l = grassLoader->addLayer(tGrassMat);

		//Configure the grass layer properties (size, density, animation properties, fade settings, etc.)
		l->setMinimumSize(5, 5);
		l->setMaximumSize(8, 8);
		if (mDef->hasVertexProgram)
		{
			l->setAnimationEnabled(true);		//Enable animations
			l->setSwayDistribution(0.05f);		//Sway fairly unsynchronized
			l->setSwayLength(0.7f);				//Sway back and forth 0.5 units in length
			l->setSwaySpeed(0.5f);				//Sway 1/2 a cycle every second
		}
		l->setDensity(0.05f);				//Relatively dense grass
		l->setRenderTechnique(GRASSTECH_CROSSQUADS);
		l->setFadeTechnique(FADETECH_GROW);	//Distant grass should slowly raise out of the ground when coming in range

		//[NOTE] This sets the color map, or lightmap to be used for grass. All grass will be colored according
		//to this texture. In this case, the colors of the terrain is used so grass will be shadowed/colored
		//just as the terrain is (this usually makes the grass fit in very well).


		//This sets the density map that will be used to determine the density levels of grass all over the
		//terrain. This can be used to make grass grow anywhere you want to; in this case it's used to make
		//grass grow only on fairly level ground (see densitymap.png to see how this works).
		l->setDensityMap(tGrassMap);

		//setMapBounds() must be called for the density and color maps to work (otherwise GrassLoader wouldn't
		//have any knowledge of where you want the maps to be applied). In this case, the maps are applied
		//to the same boundaries as the terrain.
		l->setMapBounds(TBounds(0, 0, worldSize.x, worldSize.y));
	}

	if (!mDef->pagedGeometryOn)initializeStaticGeometry();

	if (tTreesFilename.size()>0 || tTree1 != "" || tTree2 != "" || tTree3 != "")
	{
		//-------------------------------------- LOAD TREES --------------------------------------
		TreeLoader2D *treeLoader = 0;
		TreeLoader2D *largeTreeLoader = 0;
		Entity *tree1 = 0;
		Entity *tree2 = 0;
		Entity *tree3 = 0;
		bool hasLargeTrees = false;

		// УДАЛЯЕМ СТАРЫЕ ИМПОСТЕРЫ ПЕРЕД ЗАГРУЗКОЙ НОВОГО МИРА
		clearOldImpostors();

		if (mDef->pagedGeometryOn)
		{
			// ПРОСТОЙ РАБОЧИЙ ВАРИАНТ - без сложных настроек
			mTrees = new PagedGeometry();
			mTrees->setCamera(camera);
			mTrees->setPageSize(250);

			// УРОВНИ ДЕТАЛИЗАЦИИ ДЛЯ ДЕРЕВЬЕВ - оригинальный код
			mTrees->addDetailLevel<BatchPage>(300, 60);
			mTrees->addDetailLevel<BatchPage>(600, 100);
			mTrees->addDetailLevel<ImpostorPage>(1200, 150);
			mTrees->addDetailLevel<ImpostorPage>(2500, 200);

			// СОЗДАЕМ TreeLoader2D
			treeLoader = new TreeLoader2D(mTrees, TBounds(0, 0, worldSize.x, worldSize.y));
			mTrees->setPageLoader(treeLoader);
			treeLoader->setHeightFunction(&HeightFunction::getTerrainHeight);

			mLargeTrees = new PagedGeometry(camera, 250);

			// УРОВНИ ДЕТАЛИЗАЦИИ ДЛЯ БОЛЬШИХ ДЕРЕВЬЕВ
			mLargeTrees->addDetailLevel<BatchPage>(400, 80);
			mLargeTrees->addDetailLevel<BatchPage>(800, 120);
			mLargeTrees->addDetailLevel<ImpostorPage>(1500, 180);
			mLargeTrees->addDetailLevel<ImpostorPage>(3000, 250);

			largeTreeLoader = new TreeLoader2D(mLargeTrees, TBounds(0, 0, worldSize.x, worldSize.y));
			mLargeTrees->setPageLoader(largeTreeLoader);
			largeTreeLoader->setHeightFunction(&HeightFunction::getTerrainHeight);

			// ПОСЛЕ ЗАГРУЗКИ ПЕРЕМЕЩАЕМ ИМПОСТЕРЫ В ПАПКУ
			// Это вызовется позже, после загрузки всех деревьев
		}

		//Random Trees
		if (tTree1 != ""){ tree1 = mSceneMgr->createEntity("Tree1", tTree1); numTreeMeshes++; }
		if (tTree2 != ""){ tree2 = mSceneMgr->createEntity("Tree2", tTree2); numTreeMeshes++; }
		if (tTree3 != ""){ tree3 = mSceneMgr->createEntity("Tree3", tTree3); numTreeMeshes++; }

		if (tTree1 != "" || tTree2 != "" || tTree3 != "")
		{
			Ogre::Vector3 position = Ogre::Vector3::ZERO;
			Radian yaw;
			Real scale;
			for (int i = 0; i < tTreeCount; i++){
				yaw = Degree(Math::RangeRandom(0, 360));

				position.x = Math::RangeRandom(0, worldSize.x);
				position.z = Math::RangeRandom(0, worldSize.y);

				scale = Math::RangeRandom(0.8f, 1.0f);

				//[NOTE] Unlike TreeLoader3D, TreeLoader2D's addTree() function accepts a Vector2D position (x/z)
				//The Y value is calculated during runtime (to save memory) from the height function supplied (above)
				const float rnd = Math::UnitRandom();
				if (rnd<0.33 && tTree1 != "")
				{
					if (mDef->pagedGeometryOn)treeLoader->addTree(tree1, position, yaw, scale);
					else
					{
						position.y = HeightFunction::getTerrainHeight(position.x, position.z);
						Quaternion orientation;
						orientation.FromAngleAxis(yaw, Vector3::UNIT_Y);
						addStaticGeometry(tree1, position, orientation, Vector3(scale, scale, scale));
					}
				}
				else if (rnd<0.66 && tTree2 != "")
				{
					if (mDef->pagedGeometryOn)treeLoader->addTree(tree2, position, yaw, scale);
					else
					{
						position.y = HeightFunction::getTerrainHeight(position.x, position.z);
						Quaternion orientation;
						orientation.FromAngleAxis(yaw, Vector3::UNIT_Y);
						addStaticGeometry(tree2, position, orientation, Vector3(scale, scale, scale));
					}
				}
				else if (tTree3 != "")
				{
					if (mDef->pagedGeometryOn)treeLoader->addTree(tree3, position, yaw, scale);
					else
					{
						position.y = HeightFunction::getTerrainHeight(position.x, position.z);
						Quaternion orientation;
						orientation.FromAngleAxis(yaw, Vector3::UNIT_Y);
						addStaticGeometry(tree3, position, orientation, Vector3(scale, scale, scale));
					}
				}
			}
		}
		//Positioned Trees
		for (int h = 0; h<(int)tTreesFilename.size(); h++)
		{
			const String tObjects = mDef->loadWorldObjects("media/terrains/" + worldName + "/" + tTreesFilename[h].first);
			if (tObjects.length()>0)
			{
				const vector<String>::type tPart = StringUtil::split(tObjects, "[#");
				for (int i = 0; i<(int)tPart.size(); i++)
				{
					const vector<String>::type tLine = StringUtil::split(tPart[i], "\n");
					if (tLine.size()>1)
					{
						numTreeMeshes++;
						String tMesh = tLine[0];
						const vector<String>::type tMeshMat = StringUtil::split(tMesh, ";]");
						if (tMeshMat.size()>0)tMesh = tMeshMat[0];
						Entity *tree = 0;
						tree = mSceneMgr->createEntity("Tree" + StringConverter::toString(numTreeMeshes), tMesh);
						if (tMeshMat.size()>1)
						{
							for (int j = 1; j<(int)tMeshMat.size(); j++)
							{
								if (StringUtil::endsWith(tMeshMat[j], ")"))
								{
									const vector<String>::type tMat = StringUtil::split(tMeshMat[j], "()");
									if (tMat.size() >= 2)
									{
										SubEntity *tSub = tree->getSubEntity(StringConverter::parseInt(tMat[1]));
										if (tSub)tSub->setMaterialName(tMat[0]);
									}
								}
								else
								{
									tree->setMaterialName(tMeshMat[j]);
									break;
								}
							}
						}
						for (int j = 1; j<(int)tLine.size(); j++)
						{
							const vector<String>::type tSegment = StringUtil::split(tLine[j], ";");
							if (tSegment.size() == 3)
							{
								Vector3 position;
								if (tTreesFilename[h].second)position = StringConverter::parseVector3(tSegment[0]);
								else
								{
									const Vector2 p = StringConverter::parseVector2(tSegment[0]);
									position = Vector3(p.x, 0, p.y);
								}
								Vector3 scale;
								if (tTreesFilename[h].second)scale = StringConverter::parseVector3(tSegment[1]);
								else scale = Vector3(1, 1, 1)*StringConverter::parseReal(tSegment[1]);
								Vector3 pyr;
								if (tTreesFilename[h].second)pyr = StringConverter::parseVector3(tSegment[2]);
								else pyr = Vector3(0, StringConverter::parseReal(tSegment[2]), 0);
								if (mDef->pagedGeometryOn)
								{
									if (scale.y >= 2)
									{
										hasLargeTrees = true;
										if (scale.y>largeTreeLoader->getMaximumScale())largeTreeLoader->setMaximumScale(scale.y);
										largeTreeLoader->addTree(tree, position, Degree(pyr.y), scale.y);
									}
									else treeLoader->addTree(tree, position, Degree(pyr.y), scale.y);
								}
								else
								{
									if (!tTreesFilename[h].second)position.y = HeightFunction::getTerrainHeight(position.x, position.z);
									Matrix3 m; m.FromEulerAnglesXYZ(Degree(pyr.x), Degree(pyr.y), Degree(pyr.z));
									Quaternion orientation;
									orientation.FromRotationMatrix(m);
									addStaticGeometry(tree, position, orientation, scale);
								}
							}
						}
					}
				}
			}
		}
		if (!hasLargeTrees && mLargeTrees)
		{
			delete mLargeTrees->getPageLoader();
			delete mLargeTrees;
			mLargeTrees = 0;
		}
	}

	if (tBushesFilename.size()>0 || tBush1 != "" || tBush2 != "" || tBush3 != "")
	{
		//-------------------------------------- LOAD BUSHES --------------------------------------
		TreeLoader2D *bushLoader = 0;
		Entity *bush1 = 0;
		Entity *bush2 = 0;
		Entity *bush3 = 0;
		//Create and configure a new PagedGeometry instance for bushes
		//Create and configure a new PagedGeometry instance for bushes
		if (mDef->pagedGeometryOn)
		{
			mBushes = new PagedGeometry(camera, 100);
			// ДОБАВЛЕНО: УРОВНИ ДЕТАЛИЗАЦИИ ДЛЯ КУСТАРНИКОВ
			mBushes->addDetailLevel<BatchPage>(80, 15);        // 0-80: 3D кусты (очень близко)
			mBushes->addDetailLevel<BatchPage>(200, 40);       // 80-200: 3D кусты (близко)
			mBushes->addDetailLevel<ImpostorPage>(600, 80);    // 200-600: импостеры кустов (далеко)
			mBushes->addDetailLevel<ImpostorPage>(1500, 150);  // 600-1500: импостеры (очень далеко)

			//Create a new TreeLoader2D object for the bushes
			bushLoader = new TreeLoader2D(mBushes, TBounds(0, 0, worldSize.x, worldSize.y));
			mBushes->setPageLoader(bushLoader);

			//Supply the height function to TreeLoader2D so it can calculate tree Y values
			//HeightFunction::initialize(mSceneMgr);
			bushLoader->setHeightFunction(&HeightFunction::getTerrainHeight);

			//bushLoader->setColorMap("terrain_lightmap.jpg");
		}
		//Random Bushes
		if (tBush1 != ""){ bush1 = mSceneMgr->createEntity("Bush1", tBush1); numBushMeshes++; }
		if (tBush2 != ""){ bush2 = mSceneMgr->createEntity("Bush2", tBush2); numBushMeshes++; }
		if (tBush3 != ""){ bush3 = mSceneMgr->createEntity("Bush3", tBush3); numBushMeshes++; }


		if (tBush1 != "" || tBush2 != "" || tBush3 != "")
		{
			Ogre::Vector3 position = Ogre::Vector3::ZERO;
			Radian yaw;
			Real scale;
			for (int i = 0; i < worldSize.x; i++){
				yaw = Degree(Math::RangeRandom(0, 360));
				position.x = Math::RangeRandom(0, worldSize.x);
				position.z = Math::RangeRandom(0, worldSize.y);
				scale = Math::RangeRandom(0.6f, 1.0f);

				const float rnd = Math::UnitRandom();
				if (rnd<0.33 && tBush1 != "")
				{
					if (mDef->pagedGeometryOn)bushLoader->addTree(bush1, position, yaw, scale);
					else
					{
						position.y = HeightFunction::getTerrainHeight(position.x, position.z);
						Quaternion orientation;
						orientation.FromAngleAxis(yaw, Vector3::UNIT_Y);
						addStaticGeometry(bush1, position, orientation, Vector3(scale, scale, scale));
					}
				}
				else if (rnd<0.66 && tBush2 != "")
				{
					if (mDef->pagedGeometryOn)bushLoader->addTree(bush2, position, yaw, scale);
					else
					{
						position.y = HeightFunction::getTerrainHeight(position.x, position.z);
						Quaternion orientation;
						orientation.FromAngleAxis(yaw, Vector3::UNIT_Y);
						addStaticGeometry(bush2, position, orientation, Vector3(scale, scale, scale));
					}
				}
				else if (tBush3 != "")
				{
					if (mDef->pagedGeometryOn)bushLoader->addTree(bush3, position, yaw, scale);
					else
					{
						position.y = HeightFunction::getTerrainHeight(position.x, position.z);
						Quaternion orientation;
						orientation.FromAngleAxis(yaw, Vector3::UNIT_Y);
						addStaticGeometry(bush3, position, orientation, Vector3(scale, scale, scale));
					}
				}
			}
		}
		//Positioned Bushes
		for (int h = 0; h<(int)tBushesFilename.size(); h++)
		{
			const String tObjects = mDef->loadWorldObjects("media/terrains/" + worldName + "/" + tBushesFilename[h].first);
			if (tObjects.length()>0)
			{
				const vector<String>::type tPart = StringUtil::split(tObjects, "[#");
				for (int i = 0; i<(int)tPart.size(); i++)
				{
					const vector<String>::type tLine = StringUtil::split(tPart[i], "\n");
					if (tLine.size()>1)
					{
						numBushMeshes++;
						String tMesh = tLine[0];
						const vector<String>::type tMeshMat = StringUtil::split(tMesh, ";]");
						if (tMeshMat.size()>0)tMesh = tMeshMat[0];
						Entity *bush = 0;
						bush = mSceneMgr->createEntity("Bush" + StringConverter::toString(numBushMeshes), tMesh);
						if (tMeshMat.size()>1)
						{
							for (int j = 1; j<(int)tMeshMat.size(); j++)
							{
								if (StringUtil::endsWith(tMeshMat[j], ")"))
								{
									const vector<String>::type tMat = StringUtil::split(tMeshMat[j], "()");
									if (tMat.size() >= 2)
									{
										SubEntity *tSub = bush->getSubEntity(StringConverter::parseInt(tMat[1]));
										if (tSub)tSub->setMaterialName(tMat[0]);
									}
								}
								else
								{
									bush->setMaterialName(tMeshMat[j]);
									break;
								}
							}
						}
						for (int j = 1; j<(int)tLine.size(); j++)
						{
							const vector<String>::type tSegment = StringUtil::split(tLine[j], ";");
							if (tSegment.size() == 3)
							{
								Vector3 position;
								if (tBushesFilename[h].second)position = StringConverter::parseVector3(tSegment[0]);
								else
								{
									const Vector2 p = StringConverter::parseVector2(tSegment[0]);
									position = Vector3(p.x, 0, p.y);
								}
								Vector3 scale;
								if (tBushesFilename[h].second)scale = StringConverter::parseVector3(tSegment[1]);
								else scale = Vector3(1, 1, 1)*StringConverter::parseReal(tSegment[1]);
								Vector3 pyr;
								if (tBushesFilename[h].second)pyr = StringConverter::parseVector3(tSegment[2]);
								else pyr = Vector3(0, StringConverter::parseReal(tSegment[2]), 0);
								if (mDef->pagedGeometryOn)bushLoader->addTree(bush, position, Degree(pyr.y), scale.y);
								else
								{
									if (!tBushesFilename[h].second)position.y = HeightFunction::getTerrainHeight(position.x, position.z);
									Matrix3 m; m.FromEulerAnglesXYZ(Degree(pyr.x), Degree(pyr.y), Degree(pyr.z));
									Quaternion orientation;
									orientation.FromRotationMatrix(m);
									addStaticGeometry(bush, position, orientation, scale);
								}
							}
						}
					}
				}
			}
		}
	}

	for (int h = 0; h<(int)tFloatingBushesFilename.size(); h++)
	{
		//-------------------------------------- LOAD FLOATING BUSHES --------------------------------------
		TreeLoader3D *bushLoader = 0;
		if (mDef->pagedGeometryOn)
		{
			//Create and configure a new PagedGeometry instance for bushes
			mFloatingBushes = new PagedGeometry(camera, 100);
			mFloatingBushes->addDetailLevel<BatchPage>(1000, (Real)(mDef->hasVertexProgram ? 200 : 0));

			bushLoader = new TreeLoader3D(mFloatingBushes, TBounds(0, 0, worldSize.x, worldSize.y));
			mFloatingBushes->setPageLoader(bushLoader);
		}

		const String tObjects = mDef->loadWorldObjects("media/terrains/" + worldName + "/" + tFloatingBushesFilename[h].first);
		if (tObjects.length()>0)
		{
			const vector<String>::type tPart = StringUtil::split(tObjects, "[#");
			for (int i = 0; i<(int)tPart.size(); i++)
			{
				const vector<String>::type tLine = StringUtil::split(tPart[i], "\n");
				if (tLine.size()>1)
				{
					numBushMeshes++;
					String tMesh = tLine[0];
					const vector<String>::type tMeshMat = StringUtil::split(tMesh, ";]");
					if (tMeshMat.size()>0)tMesh = tMeshMat[0];
					Entity *bush = 0;
					bush = mSceneMgr->createEntity("Bush" + StringConverter::toString(numBushMeshes), tMesh);
					if (tMeshMat.size()>1)
					{
						for (int j = 1; j<(int)tMeshMat.size(); j++)
						{
							if (StringUtil::endsWith(tMeshMat[j], ")"))
							{
								const vector<String>::type tMat = StringUtil::split(tMeshMat[j], "()");
								if (tMat.size() >= 2)
								{
									SubEntity *tSub = bush->getSubEntity(StringConverter::parseInt(tMat[1]));
									if (tSub)tSub->setMaterialName(tMat[0]);
								}
							}
							else
							{
								bush->setMaterialName(tMeshMat[j]);
								break;
							}
						}
					}
					for (int j = 1; j<(int)tLine.size(); j++)
					{
						const vector<String>::type tSegment = StringUtil::split(tLine[j], ";");
						if (tSegment.size() == 3)
						{
							const Vector3 position = StringConverter::parseVector3(tSegment[0]);
							Vector3 scale;
							if (tFloatingBushesFilename[h].second)scale = StringConverter::parseVector3(tSegment[1]);
							else scale = Vector3(1, 1, 1)*StringConverter::parseReal(tSegment[1]);
							Vector3 pyr;
							if (tFloatingBushesFilename[h].second)pyr = StringConverter::parseVector3(tSegment[2]);
							else pyr = Vector3(0, StringConverter::parseReal(tSegment[2]), 0);
							if (mDef->pagedGeometryOn)bushLoader->addTree(bush, position, Degree(pyr.y), scale.y);
							else
							{
								Matrix3 m; m.FromEulerAnglesXYZ(Degree(pyr.x), Degree(pyr.y), Degree(pyr.z));
								Quaternion orientation;
								orientation.FromRotationMatrix(m);
								addStaticGeometry(bush, position, orientation, scale);
							}
						}
					}
				}
			}
		}
	}
	if (!mDef->pagedGeometryOn)buildStaticGeometry();
}

void MagixWorld::unloadWorld()
{
	isArena = false;
	worldName = "";
	isInterior = false;
	mapEffect = "";

	// ДОБАВЬ ОЧИСТКУ RTT ПРИ ВЫГРУЗКЕ МИРА:
	if (mReflectRtt)
	{
		mReflectRtt->removeAllViewports();
		mReflectRtt = 0;
	}
	if (!mReflectTex.isNull())
	{
		mReflectTex.setNull();
	}

	if (mGrass)
	{
		delete mGrass->getPageLoader();
		delete mGrass;
		mGrass = 0;
	}
	if (mTrees)
	{
		delete mTrees->getPageLoader();
		delete mTrees;
		mTrees = 0;
	}
	if (mBushes)
	{
		delete mBushes->getPageLoader();
		delete mBushes;
		mBushes = 0;
	}
	if (mLargeTrees)
	{
		delete mLargeTrees->getPageLoader();
		delete mLargeTrees;
		mLargeTrees = 0;
	}
	if (mFloatingBushes)
	{
		delete mFloatingBushes->getPageLoader();
		delete mFloatingBushes;
		mFloatingBushes = 0;
	}
	if (mSceneMgr)
	{
		destroyStaticGeometry();
		for (int i = 1; i <= numTreeMeshes; i++)
			if (mSceneMgr->hasEntity("Tree" + StringConverter::toString(i)))mSceneMgr->destroyEntity("Tree" + StringConverter::toString(i));
		numTreeMeshes = 0;
		for (int i = 1; i <= numBushMeshes; i++)
			if (mSceneMgr->hasEntity("Bush" + StringConverter::toString(i)))mSceneMgr->destroyEntity("Bush" + StringConverter::toString(i));
		numBushMeshes = 0;
		destroyAllWaterPlanes();
		destroyAllObjects();
		destroyAllParticles();
		destroyAllLights();
		destroyAllBillboards();
		mCollisionManager->destroyAllPortals();
		mCollisionManager->destroyAllGates();
		mCollisionManager->destroyAllWaterBoxes();
		mCeilingNode->setVisible(false);
	}
}

const Vector3 MagixWorld::getSpawnSquare()
{
	return spawnSquare;
}

const Vector2 MagixWorld::getWorldSize()
{
	return worldSize;
}

const Vector2 MagixWorld::getWorldBounds()
{
	return worldBounds;
}

const String MagixWorld::getWorldName()
{
	return worldName;
}

void MagixWorld::addWaterPlane(const Vector3 &position, const Real &scaleX, const Real &scaleZ,
	const String &matName, const String &playSound, bool tIsSolid)
{
	numWaterPlanes += 1;

	Entity* waterPlaneEnt = mSceneMgr->createEntity("WorldWaterPlane" + StringConverter::toString(numWaterPlanes), "WaterPlaneMesh");

	// ТОЛЬКО ДЛЯ ПЕРВОЙ ВОДНОЙ ПОВЕРХНОСТИ - СОЗДАЕМ RTT ТЕКСТУРУ
	if (numWaterPlanes == 1)
	{
		// Создаем RTT текстуру для отражений
		mReflectTex = TextureManager::getSingleton().createManual(
			"RttTex",
			ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			TEX_TYPE_2D,
			1024, 1024,
			0,
			PF_R8G8B8,
			TU_RENDERTARGET);

		mReflectRtt = mReflectTex->getBuffer()->getRenderTarget();
		mReflectRtt->setAutoUpdated(true);
		mReflectRtt->setActive(true);

		// Создаем вьюпорт с камерой отражений из MagixCamera
		Camera* reflectCam = mMainCamera->getReflectCamera();
		if (reflectCam)
		{
			Viewport *v = mReflectRtt->addViewport(reflectCam);
			v->setClearEveryFrame(true);
			v->setBackgroundColour(ColourValue(0.1f, 0.2f, 0.3f));
			v->setOverlaysEnabled(false);
			v->setShadowsEnabled(false);
		}
	}

	waterPlaneEnt->setMaterialName(matName);
	waterPlaneEnt->setCastShadows(false);
	waterPlaneEnt->setQueryFlags(WATER_MASK);

	SceneNode* waterNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
	waterNode->attachObject(waterPlaneEnt);
	waterNode->setPosition(position);
	waterNode->scale(scaleX, 1, scaleZ);

	if (playSound != "" && playSound != "no sound")
		mSoundManager->playLoopedSound(playSound.c_str(), waterNode);

	// =========== СИСТЕМА ЛЬДА - СОХРАНЯЕМ ИНФОРМАЦИЮ О ВОДНОЙ ПЛОСКОСТИ ===========
	WaterPlaneInfo info;
	info.position = position;
	info.scaleX = scaleX * 500;  // переводим в реальные размеры (WaterPlaneMesh = 500x500)
	info.scaleZ = scaleZ * 500;
	info.material = matName;
	info.isSolid = tIsSolid;

	// ОПРЕДЕЛЯЕМ ТРЕНИЕ ПО МАТЕРИАЛУ (ИСПРАВЛЕННЫЕ ЗНАЧЕНИЯ)
	if (matName.find("ice") != String::npos ||
		matName.find("Ice") != String::npos ||
		matName == "def_icewater" ||
		matName == "def_icywater" ||
		matName == "Terrain/FrozenWater")
	{
		info.friction = 0.3f;  // ОЧЕНЬ МАЛЕНЬКОЕ трение на льду = ОЧЕНЬ СКОЛЬЗКО
	}
	else if (matName.find("magma") != String::npos ||
		matName.find("Magma") != String::npos ||
		matName == "def_magmawatersmall" ||
		matName == "def_magmawaterlarge" ||
		matName == "Terrain/Magma" ||
		matName == "Terrain/LargeMagma")
	{
		info.friction = 0.8f;  // Меньше трения на магме = немного скользко
	}
	else
	{
		info.friction = 1.0f;  // НОРМАЛЬНОЕ трение на обычной поверхности
	}

	waterPlanesInfo.push_back(info);
	// =========== КОНЕЦ СИСТЕМЫ ЛЬДА ===========
}

void MagixWorld::addObject(const String &meshName, const Vector3 &position, const Vector3 &scale, const Real &pitch, const Real &yaw, const Real &roll, const String &playSound, const String &matName)
{
	numObjects += 1;

	Entity* ent = mSceneMgr->createEntity("WorldObject" + StringConverter::toString(numObjects), meshName);
	SceneNode* node = mSceneMgr->getRootSceneNode()->createChildSceneNode();

	node->attachObject(ent);
	node->setPosition(position);
	node->scale(scale);
	if (pitch != 0)node->pitch(Degree(pitch));
	if (yaw != 0)node->yaw(Degree(yaw));
	if (roll != 0)node->roll(Degree(roll));
	if (playSound != ""&&playSound != "no sound")mSoundManager->playLoopedSound(playSound.c_str(), node);
	if (matName != "")ent->setMaterialName(matName);
}

void MagixWorld::addParticle(const String &sysName, const Vector3 &position, const String &playSound)
{
	numParticles += 1;

	ParticleSystem *pSys = mSceneMgr->createParticleSystem("WorldParticle" + StringConverter::toString(numParticles), sysName);
	SceneNode* pSysNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();

	pSysNode->attachObject(pSys);
	pSysNode->setPosition(position);
	if (playSound != ""&&playSound != "no sound")mSoundManager->playLoopedSound(playSound.c_str(), pSysNode);
}

void MagixWorld::addLight(const Vector3 &position, const ColourValue &colour)
{
	if (numLights >= MAX_LIGHTS)return;
	mLight[numLights] = mSceneMgr->createLight("WorldLight" + StringConverter::toString(numLights));
#pragma warning(push)
#pragma warning(disable : 4482)
	mLight[numLights]->setType(Light::LightTypes::LT_POINT);
#pragma warning(pop)
	mLight[numLights]->setPosition(position);
	mLight[numLights]->setDiffuseColour(colour);
	mLight[numLights]->setSpecularColour(colour);
	mLight[numLights]->setAttenuation(700, 1, 0.007, 0);
	numLights++;
}

void MagixWorld::addBillboard(const Vector3 &position, const Vector2 &scale, const String &matName)
{
	numBillboards += 1;

	BillboardSet *set = mSceneMgr->createBillboardSet("WorldBillboard" + StringConverter::toString(numBillboards), 1);
	set->setCastShadows(false);
	set->setMaterialName(matName);
	set->setDefaultDimensions(scale.x, scale.y);
	set->createBillboard(Vector3::ZERO);
	SceneNode* node = mSceneMgr->getRootSceneNode()->createChildSceneNode();

	node->attachObject(set);
	node->setPosition(position);
}

void MagixWorld::destroyAllWaterPlanes()
{
	while (numWaterPlanes)
	{
		if (mSceneMgr->hasEntity("WorldWaterPlane" + StringConverter::toString(numWaterPlanes)))
		{
			Entity *ent = mSceneMgr->getEntity("WorldWaterPlane" + StringConverter::toString(numWaterPlanes));
			SceneNode* node = ent->getParentSceneNode();

			mSoundManager->stopLoopedSoundByNode(node);
			node->detachAllObjects();
			mSceneMgr->destroySceneNode(node->getName());
			mSceneMgr->destroyEntity(ent);
		}
		numWaterPlanes -= 1;
	}
}

void MagixWorld::destroyAllObjects()
{
	while (numObjects)
	{
		if (mSceneMgr->hasEntity("WorldObject" + StringConverter::toString(numObjects)))
		{
			Entity *ent = mSceneMgr->getEntity("WorldObject" + StringConverter::toString(numObjects));
			SceneNode* node = ent->getParentSceneNode();

			mSoundManager->stopLoopedSoundByNode(node);
			node->detachAllObjects();
			mSceneMgr->destroySceneNode(node->getName());
			mSceneMgr->destroyEntity(ent);
		}
		numObjects -= 1;
	}
}

void MagixWorld::destroyAllParticles()
{
	while (numParticles)
	{
		if (mSceneMgr->hasParticleSystem("WorldParticle" + StringConverter::toString(numParticles)))
		{
			ParticleSystem *pSys = mSceneMgr->getParticleSystem("WorldParticle" + StringConverter::toString(numParticles));
			SceneNode* node = pSys->getParentSceneNode();

			mSoundManager->stopLoopedSoundByNode(node);
			node->detachAllObjects();
			mSceneMgr->destroySceneNode(node->getName());
			mSceneMgr->destroyParticleSystem(pSys);
		}
		numParticles -= 1;
	}
}

void MagixWorld::destroyAllLights()
{
	for (int i = 0; i<MAX_LIGHTS; i++)
		if (mLight[i])
		{
			mSceneMgr->destroyLight(mLight[i]);
			mLight[i] = 0;
		}
	numLights = 0;
}

void MagixWorld::destroyAllBillboards()
{
	while (numBillboards)
	{
		if (mSceneMgr->hasBillboardSet("WorldBillboard" + StringConverter::toString(numBillboards)))
		{
			BillboardSet *set = mSceneMgr->getBillboardSet("WorldBillboard" + StringConverter::toString(numBillboards));
			SceneNode* node = set->getParentSceneNode();

			node->detachAllObjects();
			mSceneMgr->destroySceneNode(node->getName());
			mSceneMgr->destroyBillboardSet(set);
		}
		numBillboards -= 1;
	}
}

bool MagixWorld::getIsInterior()
{
	return isInterior;
}

const Real MagixWorld::getCeilingHeight()
{
	return ceilingHeight;
}

const String MagixWorld::getMapEffect()
{
	return mapEffect;
}

void MagixWorld::initializeStaticGeometry()
{
	if (!mSceneMgr)return;
	staticGeometryX = int(worldSize.x / 1000);
	staticGeometryY = int(worldSize.y / 1000);
	for (int i = 0; i<staticGeometryX; i++)
		for (int j = 0; j<staticGeometryY; j++)
		{
			StaticGeometry *sg = mSceneMgr->createStaticGeometry("WorldSG_" + StringConverter::toString(i) + "_" + StringConverter::toString(j));
			sg->setOrigin(Vector3((Real)(i * 1000), 0, (Real)(j * 1000)));
			sg->setRegionDimensions(Vector3(1000, 1000, 1000));
			sg->setRenderingDistance(mDef->viewDistance * 2000 + 500);
			sg->setCastShadows(true);
		}
}

void MagixWorld::addStaticGeometry(Entity *ent, const Vector3 &position, const Quaternion &orientation, const Vector3 &scale)
{
	if (!mSceneMgr)return;
	int i = int(position.x / 1000); if (i >= staticGeometryX)i = staticGeometryX - 1;
	int j = int(position.z / 1000); if (j >= staticGeometryY)j = staticGeometryY - 1;
	StaticGeometry *sg = mSceneMgr->getStaticGeometry("WorldSG_" + StringConverter::toString(i) + "_" + StringConverter::toString(j));
	sg->addEntity(ent, position, orientation, scale);
}

void MagixWorld::buildStaticGeometry()
{
	if (!mSceneMgr)return;
	for (int i = 0; i<staticGeometryX; i++)
		for (int j = 0; j<staticGeometryY; j++)
		{
			StaticGeometry *sg = mSceneMgr->getStaticGeometry("WorldSG_" + StringConverter::toString(i) + "_" + StringConverter::toString(j));
			sg->build();
		}
}

void MagixWorld::destroyStaticGeometry()
{
	if (!mSceneMgr)return;
	for (int i = 0; i<staticGeometryX; i++)
		for (int j = 0; j<staticGeometryY; j++)
		{
			mSceneMgr->destroyStaticGeometry("WorldSG_" + StringConverter::toString(i) + "_" + StringConverter::toString(j));
		}
}

const unsigned short MagixWorld::getCritterSpawnLimit()
{
	return critterSpawnLimit;
}

const vector<const WorldCritter>::type MagixWorld::getCritterSpawnList()
{
	return critterSpawnList;
}

const WorldCritter MagixWorld::getCritterSpawnList(const unsigned short &iID)
{
	if (iID<0 || iID >= critterSpawnList.size())return WorldCritter("", 0);
	return critterSpawnList[iID];
}

const std::pair<Vector3, Vector3> MagixWorld::getCritterRoamArea(const unsigned short &iID)
{
	if (iID<0 || iID >= critterRoamAreaList.size())return std::pair<Vector3, Vector3>(Vector3::ZERO, Vector3::ZERO);
	return critterRoamAreaList[iID];
}

void MagixWorld::clearCritterSpawnList()
{
	critterSpawnList.clear();
}

// === КРАФТ СИСТЕМА === //
void MagixWorld::initializeCraftRecipes()
{
	// Рецепт: 3 розы = Christmas Hat
	CraftRecipe *recipe1 = new CraftRecipe();
	recipe1->resultItem = "hat1.mesh";
	recipe1->requiredItems.push_back("rose.mesh");
	recipe1->requiredItems.push_back("rose.mesh");
	recipe1->requiredItems.push_back("rose.mesh");
	craftRecipes.push_back(recipe1);

	// Можно добавить больше рецептов здесь
}

bool MagixWorld::checkCraftRecipe(const std::vector<String>& items)
{
	return !getCraftResult(items).empty();
}

String MagixWorld::getCraftResult(const std::vector<String>& items)
{
	for (unsigned int i = 0; i < craftRecipes.size(); i++)
	{
		CraftRecipe *recipe = craftRecipes[i];
		std::vector<String> tempRequired = recipe->requiredItems;
		std::vector<String> tempItems = items;

		// Проверяем совпадение предметов
		bool match = true;
		for (unsigned int r = 0; r < tempRequired.size(); r++)
		{
			bool found = false;
			for (unsigned int t = 0; t < tempItems.size(); t++)
			{
				if (tempRequired[r] == tempItems[t])
				{
					found = true;
					tempItems.erase(tempItems.begin() + t);
					break;
				}
			}
			if (!found)
			{
				match = false;
				break;
			}
		}

		if (match && tempItems.empty())
		{
			return recipe->resultItem;
		}
	}
	return "";
}

void MagixWorld::addCraftingStation(SceneNode* stationNode)
{
	craftingStations.push_back(stationNode);
}

bool MagixWorld::isPlayerNearCraftingStation(const Vector3& playerPos)
{
	for (unsigned int i = 0; i < craftingStations.size(); i++)
	{
		SceneNode* station = craftingStations[i];
		float distance = playerPos.distance(station->getPosition());
		if (distance < 200)  // Дистанция взаимодействия
		{
			return true;
		}
	}
	return false;
}

SceneNode* MagixWorld::getNearestCraftingStation(const Vector3& playerPos)
{
	SceneNode* nearest = 0;
	float minDistance = 100000;

	for (unsigned int i = 0; i < craftingStations.size(); i++)
	{
		SceneNode* station = craftingStations[i];
		float distance = playerPos.distance(station->getPosition());
		if (distance < minDistance && distance < 200)
		{
			minDistance = distance;
			nearest = station;
		}
	}
	return nearest;
}
// === КОНЕЦ КРАФТ СИСТЕМЫ === //
Real MagixWorld::getSurfaceFrictionAtPosition(const Vector3 &position)
{
	Real friction = 1.0f;  // по умолчанию нормальное трение (земля)

	for (size_t i = 0; i < waterPlanesInfo.size(); i++)
	{
		WaterPlaneInfo &info = waterPlanesInfo[i];

		// ПРАВИЛЬНЫЙ РАСЧЕТ ГРАНИЦ водной плоскости
		Real halfWidthX = info.scaleX / 2.0f;
		Real halfWidthZ = info.scaleZ / 2.0f;

		// Границы водной плоскости по X и Z
		Real minX = info.position.x - halfWidthX;
		Real maxX = info.position.x + halfWidthX;
		Real minZ = info.position.z - halfWidthZ;
		Real maxZ = info.position.z + halfWidthZ;

		// Проверяем, находится ли позиция в пределах водной плоскости по XZ
		bool inXZ = (position.x >= minX && position.x <= maxX &&
			position.z >= minZ && position.z <= maxZ);

		// Проверяем высоту: игрок должен быть близко к поверхности воды
		// Более гибкая проверка высоты
		bool inY = (position.y >= info.position.y - 20.0f &&
			position.y <= info.position.y + 50.0f);

		if (inXZ && inY)
		{
			// Определяем, насколько близко к краю (для плавного перехода)
			Real distToEdgeX = std::min<Real>(fabs(position.x - minX),
				fabs(position.x - maxX));
			Real distToEdgeZ = std::min<Real>(fabs(position.z - minZ),
				fabs(position.z - maxZ));

			// Расстояние до ближайшего края
			Real distToEdge = std::min<Real>(distToEdgeX, distToEdgeZ);

			// Если близко к краю (в пределах 10% от размера), делаем плавный переход
			Real edgeThreshold = std::min<Real>(halfWidthX, halfWidthZ) * 0.1f;

			if (distToEdge < edgeThreshold)
			{
				// Плавное смешивание от специального трения к нормальному
				Real blendFactor = distToEdge / edgeThreshold;
				friction = info.friction + (1.0f - info.friction) * blendFactor;
			}
			else
			{
				friction = info.friction;
			}

			// Отладка
			static int debugCounter = 0;
			if (debugCounter++ % 60 == 0) // Каждые 60 кадров
			{
				const char* surfaceType = "NORMAL";
				if (friction < 0.4f) surfaceType = "ICE";
				else if (friction < 0.9f) surfaceType = "MAGMA";

				printf("SURFACE: %s at [%.1f, %.1f, %.1f], Friction=%.2f\n",
					surfaceType, position.x, position.y, position.z, friction);
			}

			return friction; // Возвращаем сразу, первая найденная зона
		}
	}

	// Если не на воде - возвращаем трение для земли
	// Можете здесь добавить проверку других типов поверхностей
	return friction;
}

bool MagixWorld::isPositionOnIce(const Vector3 &position)
{
	Real friction = getSurfaceFrictionAtPosition(position);
	return (friction < 0.4f);  // считаем льдом если трение меньше 0.4
}