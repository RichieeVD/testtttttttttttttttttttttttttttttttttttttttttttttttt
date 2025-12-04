#pragma once

#define MAX_LINES 200
#define MAX_CHANNELS 3
#define MAX_INPUT_LENGTH 1000

#define CHAT_INVALID 0
#define CHAT_GENERAL 1
#define CHAT_PRIVATE 2
#define CHAT_PARTY 3
#define CHAT_ADMIN 4
#define CHAT_ACTION 5
#define CHAT_PARTYACTION 6
#define CHAT_COMMAND 7
#define CHAT_LOCAL 8
#define CHAT_LOCALACTION 9
#define CHAT_EVENT 10
#define CHAT_LOCALEVENT 11
#define CHAT_MOD 12
#define CHAT_LIPSYNC 13
#define CHAT_PARTYEVENT 14

#define COMMAND_SETHOME "/sethome"
#define COMMAND_GOHOME "/gohome"
#define COMMAND_RESETHOME "/resethome"
#define COMMAND_POSITION "/position"
#define COMMAND_FRIEND "/friend "
#define COMMAND_UNFRIEND "/unfriend "
#define COMMAND_BLOCK "/block "
#define COMMAND_UNBLOCK "/unblock "
#define COMMAND_KICK "/kick "
#define COMMAND_ROLL "/roll "
#define COMMAND_CREATEITEM "/crooteitom "
#define COMMAND_GODSPEAK "/godspeak "
#define COMMAND_MODON "/modon"
#define COMMAND_MODOFF "/modoff"
#define COMMAND_EARTHQUAKE "/earthquake"
#define COMMAND_CLEARALL "/clearall"
#define COMMAND_CLEAR "/clear"
#define COMMAND_LIPSYNC "/lipsync "
#define COMMAND_GOTO "/goto "
#define COMMAND_SPAWN "/spoon "
#define COMMAND_WHERE "/where "
#define COMMAND_WHOIS "/whois "
#define COMMAND_RELEASEPET "/releasepet"
#define COMMAND_PETFOLLOW "/petfollow"
#define COMMAND_COMMANDS "/?"
#define COMMAND_MASSBLOCK "/massblock "
#define COMMAND_BAN "/ban "
#define COMMAND_PARTY "/party "
#define COMMAND_SAVECHAT "/savechat"
// KITO
#define COMMAND_SPAWNITEM "/spoonitom "
#define COMMAND_WEATHER "/weather "

#include "GameConfig.h"
#include "MagixUnitManager.h"

// Forward declaration
class MagixGUI;

class MagixChatManager
{
protected:
	vector<String>::type chatString[MAX_CHANNELS];
	vector<String>::type chatSayer[MAX_CHANNELS];
	vector<unsigned char>::type chatType[MAX_CHANNELS];
	bool hasNewLine[MAX_CHANNELS];
	unsigned char channel;

	// Для обработки буфера обмена
	bool mCtrlPressed;
	String* mChatInput; // Указатель на строку ввода

	// Ссылка на MagixGUI для доступа к системе смайликов
	MagixGUI* mGUI;

public:
	bool doLocalUsername;
	bool doGeneralCharname;

	MagixChatManager();
	~MagixChatManager();

	void reset(bool clearHistory = false);

	// Основные методы чата
	void push(const UTFString &caption, const String &sayer = "", const unsigned char &type = CHAT_GENERAL);
	const String getChatBlock(const unsigned short &lines, const Real &boxWidth, const Real &charHeight, const Real &lastOffset);
	void processInput(UTFString &caption, unsigned char &type, UTFString &param);
	void say(MagixUnitManager *unitMgr, MagixUnit *target, const UTFString &caption, const unsigned char &type = CHAT_GENERAL);
	void message(const UTFString &caption, const unsigned char &type = 0);

	// Управление каналами
	bool popHasNewLine(const unsigned char &chan);
	bool getHasNewLine(const unsigned char &chan);
	void toggleChannel();
	void setChannel(const unsigned char &value);
	const unsigned char getChannel();
	const vector<const unsigned char>::type getOtherChannels();

	// Форматирование текста
	const UTFString prefixChatLine(const UTFString &caption, const String &sayer, const unsigned char &type, bool isPostfixLength = false);
	const unsigned short getPrefixLength(const String &sayer, const unsigned char &type);
	const unsigned short getPostfixLength(const String &sayer, const unsigned char &type);
	const UTFString processChatString(const UTFString &caption, const String &sayer, const unsigned char &type, const Real &boxWidth, const Real &charHeight);

	// Сохранение логов
	void saveChatLog(const String &directory, const String &filename);

	// Система эмодзи
	void setGUI(MagixGUI* gui);
	UTFString processEmojis(const UTFString& text);
	void initializeEmojis();
	void testEmojis(); // Метод для тестирования эмодзи

	// Обработка буфера обмена
	void setChatInput(String* input);
	bool handleKeyPress(int scancode, bool controlPressed);
	bool handleKeyRelease(int scancode);
	bool pasteToChatInput(); // Переименовано для ясности
	static UTFString getClipboardText();
	void filterText(UTFString& text);
	static bool isTextAllowed(const UTFString& text);

	// Устаревший метод для обратной совместимости
	void pasteFromClipboard() { pasteToChatInput(); }
};