#include "MagixItemManager.h"
#include "MagixConst.h"
#include <fstream>

#include "Ogre.h"
using namespace Ogre;

MagixItemManager::MagixItemManager()
{
	mSceneMgr = 0;
	mDef = 0;
	mEffectsManager = 0;
	itemList.clear();
	itemStash.clear();
}
MagixItemManager::~MagixItemManager()
{
	deleteAllItems();
}
void MagixItemManager::initialize(SceneManager *sceneMgr, MagixExternalDefinitions *def, MagixEffectsManager *effectsMgr)
{
	mSceneMgr = sceneMgr;
	mDef = def;
	mEffectsManager = effectsMgr;
	itemList.clear();
}
void MagixItemManager::reset(bool clearStash)
{
	deleteAllItems();
	if (clearStash)
	{
		itemStash.clear();
	}
}
void MagixItemManager::update(const FrameEvent &evt, MagixCamera *camera)
{
	for (vector<MagixItem*>::type::iterator it = itemList.begin(); it != itemList.end(); it++)
	{
		MagixItem *tItem = *it;
		//Range check
		bool isInRange = false;
		if (camera)
		{
			if (camera->getCamera()->isAttached())isInRange = (tItem->getPosition().squaredDistance(camera->getCamera()->getDerivedPosition()) < 1000000);
			else isInRange = (tItem->getPosition().squaredDistance(camera->getCamera()->getPosition()) < 1000000);
		}
		if (isInRange && !tItem->hasEffect)
		{
			// Используем тип партикла из предмета
			mEffectsManager->createItemParticle(tItem->getObjectNode(), tItem->getParticleType());
			tItem->hasEffect = true;
		}
		else if (!isInRange && tItem->hasEffect)
		{
			mEffectsManager->destroyItemParticleByObjectNode(tItem->getObjectNode());
			tItem->hasEffect = false;
		}
	}
}
void MagixItemManager::createItem(const unsigned short &iID, const String &meshName, const Vector3 &position)
{
	deleteItem(iID);
	Entity *ent = mSceneMgr->createEntity("Item" + StringConverter::toString(iID), meshName + ".mesh");
	SceneNode *node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
	node->attachObject(ent);
	node->setPosition(position);
	ent->setQueryFlags(ITEM_MASK);
	ent->getAnimationState("Floor")->setEnabled(true);

	MagixItem *item = new MagixItem(node, iID, meshName);

	// Читаем тип партикла из itemParticle.cfg
	String particleType = "item"; // значение по умолчанию

	// Простое чтение конфига
	std::ifstream file("itemParticle.cfg");
	if (file.is_open())
	{
		String line;
		while (std::getline(file, line))
		{
			// Пропускаем комментарии и пустые строки
			if (line.empty() || line[0] == '#') continue;

			// Ищем меш в строке
			size_t pos = line.find(meshName + " = ");
			if (pos != String::npos)
			{
				// Нашли наш меш, извлекаем тип партикла
				size_t start = line.find("= ") + 2;
				if (start != String::npos)
				{
					particleType = line.substr(start);
					// Убираем возможные пробелы в конце
					size_t end = particleType.find_last_not_of(" \t\r\n");
					if (end != String::npos)
						particleType = particleType.substr(0, end + 1);
				}
				break;
			}
		}
		file.close();
	}

	item->setParticleType(particleType);
	itemList.push_back(item);
}
void MagixItemManager::deleteItem(MagixItem *tItem, const vector<MagixItem*>::type::iterator &it)
{
	if (!tItem)return;
	SceneNode *tObjectNode = tItem->getObjectNode();
	if (tObjectNode)
	{
		if (tItem->hasEffect)mEffectsManager->destroyItemParticleByObjectNode(tObjectNode);
		tObjectNode->detachAllObjects();
		mSceneMgr->destroySceneNode(tObjectNode->getName());
	}
	if (mSceneMgr->hasEntity("Item" + StringConverter::toString(tItem->getID())))mSceneMgr->destroyEntity("Item" + StringConverter::toString(tItem->getID()));
	itemList.erase(it);
	delete tItem;
}
void MagixItemManager::deleteItem(const unsigned short &iID)
{
	for (vector<MagixItem*>::type::iterator it = itemList.begin(); it != itemList.end(); it++)
	{
		MagixItem *tItem = *it;
		if (tItem->getID() == iID)
		{
			deleteItem(tItem, it);
			return;
		}
	}
}
void MagixItemManager::deleteAllItems()
{
	while (itemList.size()>0)
	{
		vector<MagixItem*>::type::iterator it = itemList.end();
		it--;
		MagixItem *tItem = *it;
		deleteItem(tItem, it);
	}
}
MagixItem* MagixItemManager::getByObjectNode(SceneNode *node)
{
	for (vector<MagixItem*>::type::iterator it = itemList.begin(); it != itemList.end(); it++)
	{
		MagixItem *tItem = *it;
		if (tItem->getObjectNode() == node)
		{
			return tItem;
		}
	}
	return 0;
}
MagixItem* MagixItemManager::getByID(const unsigned short &iID)
{
	for (vector<MagixItem*>::type::iterator it = itemList.begin(); it != itemList.end(); it++)
	{
		MagixItem *tItem = *it;
		if (tItem->getID() == iID)
		{
			return tItem;
		}
	}
	return 0;
}
const unsigned short MagixItemManager::getNumItems()
{
	return (unsigned short)itemList.size();
}
const unsigned short MagixItemManager::getNextEmptyID(const unsigned short &start)
{
	unsigned short tID = start;
	bool tFound = false;
	while (!tFound)
	{
		tFound = true;
		for (vector<MagixItem*>::type::iterator it = itemList.begin(); it != itemList.end(); it++)
		{
			MagixItem *tItem = *it;
			if (tItem->getID() == tID)
			{
				tFound = false;
				break;
			}
		}
		if (tFound)break;
		else tID++;
	}
	return tID;
}
const vector<MagixItem*>::type MagixItemManager::getItemList()
{
	return itemList;
}
bool MagixItemManager::isStashFull()
{
	return (itemStash.size() >= MAX_STASH);
}

bool MagixItemManager::pushStash(const String &name)
{
	// Проверяем, есть ли уже такой предмет и можно ли добавить еще
	for (int i = 0; i < (int)itemStash.size(); i++)
	{
		if (itemStash[i].name == name && itemStash[i].count < 5)
		{
			itemStash[i].count++;
			return true;
		}
	}

	// Если предмета нет или стак полный, создаем новый слот
	if (itemStash.size() >= MAX_STASH) return false;

	itemStash.push_back(StashItem(name, 1));
	return true;
}

const String MagixItemManager::getStashAt(const unsigned short &line)
{
	if (line >= itemStash.size()) return "";
	return itemStash[line].name;  // ИСПРАВЛЕНО: .name
}

unsigned short MagixItemManager::getStashCountAt(const unsigned short &line)
{
	if (line >= itemStash.size()) return 0;
	return itemStash[line].count;
}

const String MagixItemManager::popStash(const unsigned short &line)
{
	String tItem = "";
	unsigned short tCount = 0;

	// ИСПРАВЛЕНО: правильный тип итератора
	for (vector<StashItem>::type::iterator it = itemStash.begin(); it != itemStash.end(); it++)
	{
		if (tCount == line)
		{
			tItem = it->name;  // ИСПРАВЛЕНО: it->name
			if (it->count > 1)
			{
				it->count--;
			}
			else
			{
				itemStash.erase(it);
			}
			break;
		}
		tCount++;
	}
	return tItem;
}

int MagixItemManager::getStashSize()
{
	return (int)itemStash.size();
}

int MagixItemManager::getTotalItemCount()
{
	int total = 0;
	for (int i = 0; i < (int)itemStash.size(); i++)
	{
		total += itemStash[i].count;
	}
	return total;
}

const vector<const String>::type MagixItemManager::getStash()
{
	vector<const String>::type result;
	for (int i = 0; i < (int)itemStash.size(); i++)
	{
		result.push_back(itemStash[i].name);
	}
	return result;
}

void MagixItemManager::clearStash()
{
	itemStash.clear();
}

const String MagixItemManager::removeStashItem(const unsigned short &line)
{
	if (line >= itemStash.size()) return "";

	String tItem = itemStash[line].name;

	// Уменьшаем количество или удаляем слот
	if (itemStash[line].count > 1)
	{
		itemStash[line].count--;
	}
	else
	{
		itemStash.erase(itemStash.begin() + line);
	}

	return tItem;
}