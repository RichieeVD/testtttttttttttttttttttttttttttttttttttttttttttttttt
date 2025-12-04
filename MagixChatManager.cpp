#include "MagixChatManager.h"
#include "MagixUnit.h"
#include "MagixUnitManager.h"
#include "MagixGUI.h"
#include <sstream>
#include <vector>
#include <fstream>

#ifdef _WIN32
#include <Windows.h>
#endif

// Конструктор
MagixChatManager::MagixChatManager() :
channel(0),
doLocalUsername(false),
doGeneralCharname(false),
mCtrlPressed(false),
mChatInput(nullptr),
mGUI(nullptr)
{
	reset(true);
}

// Деструктор
MagixChatManager::~MagixChatManager()
{
}

// Сброс чата
void MagixChatManager::reset(bool clearHistory)
{
	if (clearHistory)
	{
		for (int i = 0; i < MAX_CHANNELS; i++)
		{
			chatString[i].clear();
			chatSayer[i].clear();
			chatType[i].clear();
			hasNewLine[i] = false;
		}
	}
}

// Установка ссылки на GUI
void MagixChatManager::setGUI(MagixGUI* gui)
{
	mGUI = gui;
}

// Установка указателя на ввод чата
void MagixChatManager::setChatInput(String* input)
{
	mChatInput = input;
}

// Обработка нажатия клавиши
bool MagixChatManager::handleKeyPress(int scancode, bool controlPressed)
{
	if (!mChatInput) return false;

	// Обработка Ctrl
	if (controlPressed)
	{
		mCtrlPressed = true;
		return false;
	}

	// Обработка Ctrl+V (OIS::KC_V = 48)
	if (mCtrlPressed && scancode == 48)
	{
		return pasteToChatInput();
	}

	// Обработка Ctrl+C (OIS::KC_C = 46) - для копирования
	if (mCtrlPressed && scancode == 46)
	{
		// Можно добавить функциональность копирования
		return true;
	}

	return false;
}

// Обработка отпускания клавиши
bool MagixChatManager::handleKeyRelease(int scancode)
{
	// Сбрасываем Ctrl при отпускании
	mCtrlPressed = false;
	return false;
}

// Вставка текста из буфера обмена
bool MagixChatManager::pasteToChatInput()
{
	if (!mChatInput)
	{
		message("Chat input not available");
		return false;
	}

#ifdef _WIN32
	if (!OpenClipboard(nullptr))
	{
		message("Cannot open clipboard");
		return false;
	}

	bool success = false;
	HANDLE hData = GetClipboardData(CF_UNICODETEXT);
	if (hData != nullptr)
	{
		wchar_t* text = static_cast<wchar_t*>(GlobalLock(hData));
		if (text != nullptr)
		{
			UTFString clipboardText = text;
			filterText(clipboardText);

			// Обрабатываем эмодзи в тексте из буфера обмена
			clipboardText = processEmojis(clipboardText);

			// Добавляем к текущему тексту
			*mChatInput += String(clipboardText);

			GlobalUnlock(hData);
			success = true;
		}
	}
	CloseClipboard();

	if (success)
	{
		message("Text pasted from clipboard");
	}
	else
	{
		message("No text in clipboard");
	}

	return success;
#else
	// Linux/Mac implementation would go here
	message("Clipboard not supported on this platform");
	return false;
#endif
}

// Получение текста из буфера обмена (вспомогательный метод)
UTFString MagixChatManager::getClipboardText()
{
#ifdef _WIN32
	if (!OpenClipboard(nullptr))
		return L"";

	UTFString result;
	HANDLE hData = GetClipboardData(CF_UNICODETEXT);
	if (hData != nullptr)
	{
		wchar_t* text = static_cast<wchar_t*>(GlobalLock(hData));
		if (text != nullptr)
		{
			result = text;
			GlobalUnlock(hData);
		}
	}
	CloseClipboard();
	return result;
#else
	return L"";
#endif
}

// Фильтрация текста
void MagixChatManager::filterText(UTFString& text)
{
	for (size_t i = 0; i < text.length(); ++i)
	{
		// Удаляем управляющие символы кроме табуляции и новой строки
		if (text[i] < 0x20 && text[i] != '\t' && text[i] != '\n')
		{
			text[i] = ' ';
		}
	}
}

// Проверка допустимости текста
bool MagixChatManager::isTextAllowed(const UTFString& text)
{
	for (size_t i = 0; i < text.length(); ++i)
	{
		if (text[i] < 0x20 && text[i] != '\t' && text[i] != '\n')
			return false;
	}
	return true;
}

// Обработка эмодзи в тексте
UTFString MagixChatManager::processEmojis(const UTFString& text)
{
	if (!mGUI || text.empty())
	{
		return text;
	}

	// Конвертируем UTFString в String для обработки
	String regularText = text;

	// Используем систему эмодзи из GUI
	String processed = mGUI->processTextWithEmojis(regularText);

	// Конвертируем обратно в UTFString
	UTFString result;
	result = processed;

	return result;
}

// Добавление сообщения в историю
void MagixChatManager::push(const UTFString &caption, const String &sayer, const unsigned char &type)
{
	// Обрабатываем эмодзи перед сохранением сообщения
	UTFString processedCaption = processEmojis(caption);

	const unsigned short tChannel = (type == CHAT_PRIVATE ? channel :
		((type == CHAT_LOCAL || type == CHAT_LOCALACTION || type == CHAT_LOCALEVENT) ? 0 :
		((type == CHAT_PARTY || type == CHAT_PARTYACTION || type == CHAT_PARTYEVENT) ? 2 : 1)));

	chatString[tChannel].push_back(processedCaption);
	chatSayer[tChannel].push_back(sayer);
	chatType[tChannel].push_back(type);

	// Ограничение размера истории
	if (int(chatString[tChannel].size()) > MAX_LINES)
	{
		chatString[tChannel].erase(chatString[tChannel].begin());
		chatSayer[tChannel].erase(chatSayer[tChannel].begin());
		chatType[tChannel].erase(chatType[tChannel].begin());
	}

	hasNewLine[tChannel] = true;
}

// Получение блока чата для отображения
const String MagixChatManager::getChatBlock(const unsigned short &lines, const Real &boxWidth, const Real &charHeight, const Real &lastOffset)
{
	vector<String>::type tChatBlock;
	String tFinalChat = "";
	unsigned short tFinalLines = 0;

	int start = int(chatString[channel].size()) - lines - (int)lastOffset * (int(chatString[channel].size()) - lines);
	if (start < 0) start = 0;

	for (int i = start; i < int(chatString[channel].size()); i++)
	{
		const String tLineBlock = prefixChatLine(processChatString(chatString[channel][i], chatSayer[channel][i], chatType[channel][i], boxWidth, charHeight), chatSayer[channel][i], chatType[channel][i]);
		vector<String>::type tCaption = StringUtil::split(tLineBlock, "\n");

		for (int j = 0; j < int(tCaption.size()); j++)
		{
			tChatBlock.push_back(tCaption[j]);
		}
	}

	start = int(tChatBlock.size()) - lines - (int)lastOffset * (int(tChatBlock.size()) - lines);
	if (start < 0) start = 0;

	for (int i = start; i < int(tChatBlock.size()); i++)
	{
		tFinalChat += (i == start ? "" : "\n");
		tFinalChat += tChatBlock[i];
		tFinalLines += 1;
		if (tFinalLines >= lines) return tFinalChat;
	}

	return tFinalChat;
}

// Обработка ввода пользователя
void MagixChatManager::processInput(UTFString &caption, unsigned char &type, UTFString &param)
{
	// Установка типа чата по умолчанию
	if (channel == 0) type = CHAT_LOCAL;
	if (channel == 2) type = CHAT_PARTY;
	param = "";

	// ========== КОМАНДЫ ЧАТА ==========

	// Команда для показа эмодзи
	if (StringUtil::startsWith(caption, "/emojis") || StringUtil::startsWith(caption, "/emoji"))
	{
		type = CHAT_COMMAND;
		param = "emojis";
		if (mGUI)
		{
			mGUI->printAvailableEmojis();
		}
		caption = "";
		return;
	}

	// Домашние команды
	if (StringUtil::startsWith(caption, COMMAND_SETHOME))
	{
		type = CHAT_COMMAND;
		param = COMMAND_SETHOME;
		return;
	}
	if (StringUtil::startsWith(caption, COMMAND_GOHOME))
	{
		type = CHAT_COMMAND;
		param = COMMAND_GOHOME;
		return;
	}
	if (StringUtil::startsWith(caption, COMMAND_RESETHOME))
	{
		type = CHAT_COMMAND;
		param = COMMAND_RESETHOME;
		return;
	}

	// Информационные команды
	if (StringUtil::startsWith(caption, COMMAND_POSITION))
	{
		type = CHAT_COMMAND;
		param = COMMAND_POSITION;
		return;
	}

	// Социальные команды
	if (StringUtil::startsWith(caption, COMMAND_FRIEND))
	{
		caption.erase(0, 8);
		type = CHAT_COMMAND;
		param = COMMAND_FRIEND;
		return;
	}
	if (StringUtil::startsWith(caption, COMMAND_UNFRIEND))
	{
		caption.erase(0, 10);
		type = CHAT_COMMAND;
		param = COMMAND_UNFRIEND;
		return;
	}
	if (StringUtil::startsWith(caption, COMMAND_BLOCK))
	{
		caption.erase(0, 7);
		type = CHAT_COMMAND;
		param = COMMAND_BLOCK;
		return;
	}
	if (StringUtil::startsWith(caption, COMMAND_UNBLOCK))
	{
		caption.erase(0, 9);
		type = CHAT_COMMAND;
		param = COMMAND_UNBLOCK;
		return;
	}
	if (StringUtil::startsWith(caption, COMMAND_PARTY))
	{
		caption.erase(0, 7);
		type = CHAT_COMMAND;
		param = COMMAND_PARTY;
		return;
	}

	// Административные команды
	if (StringUtil::startsWith(caption, COMMAND_KICK))
	{
		caption.erase(0, 6);
		type = CHAT_COMMAND;
		param = COMMAND_KICK;
		return;
	}
	if (StringUtil::startsWith(caption, COMMAND_BAN))
	{
		caption.erase(0, 5);
		type = CHAT_COMMAND;
		param = COMMAND_BAN;
		return;
	}
	if (StringUtil::startsWith(caption, COMMAND_MASSBLOCK))
	{
		caption.erase(0, 11);
		type = CHAT_COMMAND;
		param = COMMAND_MASSBLOCK;
		return;
	}
	if (StringUtil::startsWith(caption, COMMAND_WHERE))
	{
		caption.erase(0, 7);
		type = CHAT_COMMAND;
		param = COMMAND_WHERE;
		return;
	}
	if (StringUtil::startsWith(caption, COMMAND_WHOIS))
	{
		caption.erase(0, 7);
		type = CHAT_COMMAND;
		param = COMMAND_WHOIS;
		return;
	}

	// Модераторские команды
	if (StringUtil::startsWith(caption, COMMAND_MODON))
	{
		type = CHAT_COMMAND;
		param = COMMAND_MODON;
		return;
	}
	if (StringUtil::startsWith(caption, COMMAND_MODOFF))
	{
		type = CHAT_COMMAND;
		param = COMMAND_MODOFF;
		return;
	}

	// Игровые команды
	if (StringUtil::startsWith(caption, COMMAND_ROLL))
	{
		caption.erase(0, 6);
		type = CHAT_COMMAND;
		param = COMMAND_ROLL;
		return;
	}
	if (StringUtil::startsWith(caption, COMMAND_CREATEITEM))
	{
		caption.erase(0, 12);
		type = CHAT_COMMAND;
		param = COMMAND_CREATEITEM;
		return;
	}
	if (StringUtil::startsWith(caption, COMMAND_SPAWNITEM))
	{
		caption.erase(0, 11);
		type = CHAT_COMMAND;
		param = COMMAND_SPAWNITEM;
		return;
	}
	if (StringUtil::startsWith(caption, COMMAND_SPAWN))
	{
		caption.erase(0, 7);
		type = CHAT_COMMAND;
		param = COMMAND_SPAWN;
		return;
	}
	if (StringUtil::startsWith(caption, COMMAND_GOTO))
	{
		caption.erase(0, 6);
		type = CHAT_COMMAND;
		param = COMMAND_GOTO;
		return;
	}

	// Системные команды
	if (StringUtil::startsWith(caption, COMMAND_GODSPEAK))
	{
		caption.erase(0, 10);
		type = CHAT_COMMAND;
		param = COMMAND_GODSPEAK;
		return;
	}
	if (StringUtil::startsWith(caption, COMMAND_EARTHQUAKE))
	{
		type = CHAT_COMMAND;
		param = COMMAND_EARTHQUAKE;
		return;
	}
	if (StringUtil::startsWith(caption, COMMAND_WEATHER))
	{
		if (caption.length() > 9) caption.erase(0, 9);
		else caption = "";
		type = CHAT_COMMAND;
		param = COMMAND_WEATHER;
		return;
	}

	// Команды очистки
	if (StringUtil::startsWith(caption, COMMAND_CLEARALL))
	{
		type = CHAT_COMMAND;
		param = COMMAND_CLEARALL;
		reset(true);
		return;
	}
	if (StringUtil::startsWith(caption, COMMAND_CLEAR))
	{
		type = CHAT_COMMAND;
		param = COMMAND_CLEAR;
		chatString[channel].clear();
		chatSayer[channel].clear();
		chatType[channel].clear();
		hasNewLine[channel] = false;
		return;
	}

	// Команды питомцев
	if (StringUtil::startsWith(caption, COMMAND_RELEASEPET))
	{
		type = CHAT_COMMAND;
		param = COMMAND_RELEASEPET;
		return;
	}
	if (StringUtil::startsWith(caption, COMMAND_PETFOLLOW))
	{
		type = CHAT_COMMAND;
		param = COMMAND_PETFOLLOW;
		return;
	}

	// Специальные команды
	if (StringUtil::startsWith(caption, COMMAND_LIPSYNC))
	{
		caption.erase(0, 9);
		type = CHAT_LIPSYNC;
		return;
	}
	if (StringUtil::startsWith(caption, COMMAND_COMMANDS))
	{
		type = CHAT_COMMAND;
		param = COMMAND_COMMANDS;
		return;
	}
	if (StringUtil::startsWith(caption, COMMAND_SAVECHAT))
	{
		if (caption.length()>10) caption.erase(0, 10);
		else caption = "";
		type = CHAT_COMMAND;
		param = COMMAND_SAVECHAT;
		return;
	}

	// Действия (/me)
	if (StringUtil::startsWith(caption, "/me "))
	{
		caption.erase(0, 4);
		switch (type)
		{
		case CHAT_PARTY: type = CHAT_PARTYACTION; break;
		case CHAT_LOCAL: type = CHAT_LOCALACTION; break;
		default: type = CHAT_ACTION; break;
		}
		return;
	}

	// Приватные сообщения (/username: message)
	if (StringUtil::startsWith(caption, "/"))
	{
		const vector<String>::type tSplit = StringUtil::split(caption, ":", 1);
		if (tSplit.size()>1 && tSplit[0].length()>1)
		{
			param = tSplit[0];
			param.erase(0, 1);
			caption = tSplit[1];
			type = CHAT_PRIVATE;
		}
		else
		{
			type = CHAT_INVALID;
		}
		return;
	}
}

// Отправка сообщения от персонажа
void MagixChatManager::say(MagixUnitManager *unitMgr, MagixUnit *target, const UTFString &caption, const unsigned char &type)
{
	// Обрабатываем эмодзи перед отправкой сообщения
	UTFString processedCaption = processEmojis(caption);

	const UTFString tName = (((!doLocalUsername && (type == CHAT_LOCAL || type == CHAT_LOCALACTION || type == CHAT_LOCALEVENT || type == CHAT_PARTY || type == CHAT_PARTYACTION || type == CHAT_PARTYEVENT)) ||
		(doGeneralCharname && (type == CHAT_GENERAL || type == CHAT_ACTION || type == CHAT_EVENT)) ||
		target->getUser() == "") ? target->getName() : target->getUser());

	if ((type != CHAT_LOCAL && type != CHAT_LOCALACTION) || target->getBodyEnt()->isVisible())
	{
		push(processedCaption, tName, type);
	}

	if ((type == CHAT_LOCAL || type == CHAT_PARTY) && target->getBodyEnt()->isVisible())
	{
		unitMgr->createChatBubble(target, processedCaption);
	}
}

// Системное сообщение
void MagixChatManager::message(const UTFString &caption, const unsigned char &type)
{
	// Обрабатываем эмодзи в системных сообщениях
	UTFString processedCaption = processEmojis(caption);

	if (type == 0)
	{
		push(processedCaption, "", (channel == 0 ? CHAT_LOCAL : (channel == 1 ? CHAT_GENERAL : CHAT_PARTY)));
	}
	else
	{
		push(processedCaption, "", type);
	}
}

// Проверка наличия новых сообщений
bool MagixChatManager::popHasNewLine(const unsigned char &chan)
{
	const bool tNewLine = hasNewLine[chan];
	hasNewLine[chan] = false;
	return tNewLine;
}

bool MagixChatManager::getHasNewLine(const unsigned char &chan)
{
	return hasNewLine[chan];
}

// Форматирование строки чата с префиксом
const UTFString MagixChatManager::prefixChatLine(const UTFString &caption, const String &sayer, const unsigned char &type, bool isPostfixLength)
{
	UTFString tLine = "";
	if (sayer != "" && !isPostfixLength)
	{
		if (type == CHAT_ADMIN) tLine = L"" + tLine + "(Admin)";
		else if (type == CHAT_MOD) tLine = L"" + tLine + "(Mod)";

		if (type == CHAT_ACTION || type == CHAT_PARTYACTION || type == CHAT_LOCALACTION)
		{
			tLine = L"" + tLine + sayer + " ";
		}
		else if (type == CHAT_PRIVATE)
		{
			tLine = L"" + tLine + "((" + sayer + ")) ";
		}
		else if (type == CHAT_EVENT || type == CHAT_LOCALEVENT || type == CHAT_PARTYEVENT)
		{
			tLine = L"" + tLine + "[" + sayer + " ";
		}
		else
		{
			tLine = L"" + tLine + "<" + sayer + "> ";
		}
	}

	tLine = L"" + tLine + caption;

	if (sayer != "" && (caption != "" || isPostfixLength))
	{
		if (type == CHAT_EVENT || type == CHAT_LOCALEVENT || type == CHAT_PARTYEVENT)
		{
			tLine = L"" + tLine + "]";
		}
	}

	return tLine;
}

// Получение длины префикса
const unsigned short MagixChatManager::getPrefixLength(const String &sayer, const unsigned char &type)
{
	return (int)prefixChatLine("", sayer, type, false).length();
}

// Получение длины постфикса
const unsigned short MagixChatManager::getPostfixLength(const String &sayer, const unsigned char &type)
{
	return (int)prefixChatLine("", sayer, type, true).length();
}

// Обработка строки чата для отображения
const UTFString MagixChatManager::processChatString(const UTFString &caption, const String &sayer, const unsigned char &type, const Real &boxWidth, const Real &charHeight)
{
	// Эмодзи уже обработаны при push(), просто форматируем текст

	UTFString tCaption = prefixChatLine(caption, sayer, type);
	const unsigned short tPrefix = getPrefixLength(sayer, type);
	const unsigned short tPostfix = getPostfixLength(sayer, type);

	if (tCaption.length() > 0)
	{
		const Font *pFont = dynamic_cast<Ogre::Font*>(Ogre::FontManager::getSingleton().getByName("Tahoma").getPointer());
		if (!pFont) return tCaption;

		const Real tHeight = charHeight;

		int tSpacePos = -1;
		Real tTextWidth = 0;
		Real tWidthFromSpace = 0;

		for (int i = 0; i < int(tCaption.length()); i++)
		{
			// Учитываем что эмодзи могут быть разной ширины
			Real charWidth = 0;

			if (tCaption[i] == 0x0020) // space
			{
				charWidth = pFont->getGlyphAspectRatio(0x0030);
			}
			else
			{
				// Для эмодзи используем стандартную ширину
				charWidth = pFont->getGlyphAspectRatio(tCaption[i]);
			}

			tTextWidth += charWidth;
			tWidthFromSpace += charWidth;

			if (tCaption[i] == ' ' && i > tPrefix)
			{
				tSpacePos = i;
				tWidthFromSpace = pFont->getGlyphAspectRatio(0x0030);
			}

			if (tTextWidth * tHeight >= boxWidth)
			{
				if (tSpacePos != -1)
				{
					tCaption[tSpacePos] = '\n';
					tTextWidth = tWidthFromSpace;
					tSpacePos = -1;
				}
				else
				{
					tCaption.insert(i + 1, String(1, '\n'));
					tTextWidth = 0;
				}
			}
		}
	}

	tCaption.erase(0, tPrefix);
	tCaption.erase(tCaption.length() - tPostfix, tPostfix);
	return tCaption;
}

// Переключение канала чата
void MagixChatManager::toggleChannel()
{
	channel++;
	if (channel >= MAX_CHANNELS) channel = 0;
}

// Установка канала чата
void MagixChatManager::setChannel(const unsigned char &value)
{
	channel = value;
	if (channel >= MAX_CHANNELS) channel = MAX_CHANNELS - 1;
}

// Получение текущего канала
const unsigned char MagixChatManager::getChannel()
{
	return channel;
}

// Получение других каналов
const vector<const unsigned char>::type MagixChatManager::getOtherChannels()
{
	vector<const unsigned char>::type tChannels;
	for (int i = 0; i < MAX_CHANNELS; i++)
	{
		if (i != channel) tChannels.push_back(i);
	}
	return tChannels;
}

// Сохранение лога чата
void MagixChatManager::saveChatLog(const String &directory, const String &filename)
{
	std::ofstream outFile;
	const String tFilename = directory + filename;
	outFile.open(tFilename.c_str());

	if (outFile.good())
	{
		for (int i = 0; i < (int)chatString[channel].size(); i++)
		{
			const String tBuffer = prefixChatLine(chatString[channel][i], chatSayer[channel][i], chatType[channel][i]) + "\n";
			outFile.write(tBuffer.c_str(), (int)tBuffer.length());
		}
		message("Saved chat log as " + filename + ".");
	}
	else
	{
		message("Failed to save chat log: " + filename);
	}

	outFile.close();
}

// Инициализация эмодзи
void MagixChatManager::initializeEmojis()
{
	// Эмодзи инициализируются через GUI
	// Этот метод для совместимости
}

// Тестирование эмодзи
void MagixChatManager::testEmojis()
{
	if (!mGUI)
	{
		message("GUI not available for emoji testing");
		return;
	}

	message("=== Emoji Test ===");
	message("Smileys: :) :( :D :P ;) :O");
	message("Hearts: <3 :heart:");
	message("Objects: :fire: :star:");
	message("Expressions: :sob: :rofl:");
	message("Actions: :like:");
	message("================");
}