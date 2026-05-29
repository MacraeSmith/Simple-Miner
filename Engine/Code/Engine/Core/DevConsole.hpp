#pragma once
#include "Game/EngineBuildPreferences.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include <vector>
#include <string>
#include <mutex>




class Renderer;
class RendererDX12; 
class RendererDX11;
#ifdef ERROR
#undef ERROR
#endif
struct AABB2;
class BitmapFont;
class Camera;
class Timer;
class ChatSystem;

constexpr float DEFAULT_TEXT_HEIGHT = 0.75f;
constexpr float DEFAULT_LINES_ON_SCREEN = 29.5f;
enum DevConsoleMode : int
{
	HIDDEN,
	FULL,
	NUM_DEVCONSOLE_MODES,
};

struct DevConsoleConfig
{
	Renderer* m_defaultRenderer = nullptr;
	std::string m_defaultFontName;
	float m_defaultFontAspect = 1.f;
	float m_defaultLinesOnScreen = DEFAULT_LINES_ON_SCREEN;
	float m_numLinesOnScreen = m_defaultLinesOnScreen;
	bool m_activateChatSystem = false;
};

enum DevConsoleLineType : int 
{
	INVALID_LINE_TYPE = -1,
	COMMAND,
	COMMAND_REMOTE,
	COPYABLE_MESSAGE,
	COMMAND_MESSAGE,
	CHAT_MESSAGE,
	CHAT_MESSAGE_REMOTE,
	ALERT_MESSAGE,
	NUM_DEVCONSOLE_LINE_TYPES
};

struct DevConsoleLine
{
	Rgba8 m_color = Rgba8::WHITE;
	std::string m_text;
	int m_frameNumberPrinted = 0;
	double m_timePrinted = 0.0;
	float m_textHeightScale = DEFAULT_TEXT_HEIGHT;
	DevConsoleLineType m_lineType = COMMAND;
	bool IsCopyable() const {return m_lineType == COMMAND || m_lineType == COPYABLE_MESSAGE;}
	bool IsChat() const {return m_lineType == CHAT_MESSAGE || m_lineType == CHAT_MESSAGE_REMOTE;}
};

struct HelpBox
{
	bool m_shouldRender = false;
	Strings m_text;
	std::string m_longestLine;
	int m_startIndex = 0;
};

struct DevConsoleInputLine
{
	Rgba8 m_color = Rgba8(175, 175, 175);
	std::string m_text;
	bool m_validCommand = false;
};

class DevConsole
{
friend class ChatSystem;
public:
	explicit DevConsole(DevConsoleConfig const& config);
	~DevConsole() {}
	void Startup();
	void ShutDown();
	void BeginFrame();
	void EndFrame();
	void Render(AABB2 const& screenBounds, Renderer* overrideRenderer = nullptr) const;
	void Render(Camera* camera, Renderer* overrideRenderer = nullptr) const;

	void Execute(std::string const& consoleCommandText);
	void AddLine(Rgba8 const& color, std::string const& text, float heightPercent = 1.f, bool isMessage = false);
	void AddLine(Rgba8 const& color, std::string const& text, DevConsoleLineType lineType, float heightPercent = 1.f);
	void AddLine(DevConsoleLineType lineType, std::string const& text, Rgba8 const& color = Rgba8::BLACK, float overrideHeightPercent = -1.f);
	void AddLineAndExecute(DevConsoleLineType lineType, std::string const& text, Rgba8 const& color = Rgba8::BLACK, float overrideHeightPercent = -1.f);
	void RemoveLastLine();

	DevConsoleMode GetMode() const;
	void SetMode(DevConsoleMode mode);
	void ToggleMode(DevConsoleMode mode);
	bool IsOpen() const;

	std::string GetTextFromMostRecentLine();

	bool Event_KeyPressed(EventArgs& args);
	bool Event_CharInput(EventArgs& args);
	bool Event_PasteRequested(EventArgs& args);
	bool Event_CommandClear(EventArgs& args);
	bool Event_CommandHelp(EventArgs& args);
	bool Event_CommandResizeDevConsole(EventArgs& args);
	bool Event_CommandDevConsoleNavigation(EventArgs& args);
	bool Event_CommandShowSuggestions(EventArgs& args);
	bool Event_Echo(EventArgs& args);
	bool Event_RemoteCommand(EventArgs& args);
	bool Event_Chat(EventArgs& args);
	bool Event_Notifications(EventArgs& args);
	bool Event_ExecuteXmlCommandScriptFile(EventArgs& args);
	static DevConsoleLineType GetLineTypeFromNotificationString(std::string const& string);

	void ExecuteXmlCommandScriptFile(std::string const& commandScriptXmlFilePathName);
	void ExecuteXmlCommandScriptNode(XmlElement const& commandScriptXmlElement);

public:
	static const Rgba8 ERROR;
	static const Rgba8 WARNING;
	static const Rgba8 INFO_MAJOR;
	static const Rgba8 INFO_MINOR;
	static const Rgba8 SELECTED_LINE;
	static const Rgba8 VALID_COMMAND;
	static const Rgba8 VALID_REMOTE_COMMAND;
	static const Rgba8 VALID_LINE;
	static const Rgba8 HELP_INFO;
	static const Rgba8 CONSOLE_BACKGROUND;
	static const Rgba8 INPUT_LINE_BACKGROUND;
	static const Rgba8 CHAT;
	static const Rgba8 CHAT_REMOTE;

protected:
	void Render_Open(AABB2 const& bounds, Renderer* renderer, BitmapFont& font, float fontAspect = 1.f) const;
	void Render_Alerts(AABB2 const& bounds, Renderer* renderer, BitmapFont& font, float fontAspect = 1.f) const;

	void EditInputText(unsigned char keyCode);
	void AppendToInputText(std::string const& textToAdd);
	void DeleteFromInputText(unsigned char keyCode);
	void MoveInsertionCursor(unsigned char keyCode);
	void NavigateLines(unsigned char keyCode);
	bool ExecuteInputLine();
	void ClearCommands(EventArgs& args);
	void HelpCommand();
	void ClearInputLine();
	void ResizeConsoleWindow(EventArgs& args);
	void ResetInsertionPointBlink();
	void UpdateInputLineColor();
	void SaveHelpTextForEventNames();
	void FormatHelpBoxForEventName(std::string const& eventName);
	void ShowSuggestions(bool show);
	bool ShowAsAlert(DevConsoleLine const& line) const;

	void SanitizePastedText(std::string& pastedText) const;


protected:
	DevConsoleConfig m_config;
	DevConsoleMode m_mode = HIDDEN;
	float m_consoleWindowScale = 1.f;
	std::vector<DevConsoleLine> m_lines;
	DevConsoleInputLine m_inputLine;
	int m_insertionPointPosition = 0;
	int m_selectedLineNum = -1;
	std::vector<std::string> m_commandHistory;


	bool m_insertionPointVisible = true;
	int m_frameNumber = 0;

	Timer* m_insertionPointTimer = nullptr;

	bool m_showSuggestions = true;
	bool m_isNavigatingHelpBox = false;
	HelpBox m_helpBox;
	int m_helpBoxSelectedLinePosition = -1;
	std::map<HashedCaseInsensitiveString, Strings> m_helpTextByEventName;
	float m_scrollPercentage = 0.f;

	//Chat
	ChatSystem* m_chatSystem = nullptr;
	bool m_showOnlyChat = false;
	bool m_hideChat = false;
	int m_numChatLines = 0;
	bool m_visibleAlerts[NUM_DEVCONSOLE_LINE_TYPES] = {false, true, false, false, false, true, true};
	mutable std::recursive_mutex m_devConsoleMutex;
};
