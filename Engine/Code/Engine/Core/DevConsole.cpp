#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Network/NetworkSystem.hpp"
#include "Engine/Network/ChatSystem.hpp"
#include "Engine/Window/Window.hpp"
#include <algorithm>

#ifdef RENDERER_DX12
	#include "Engine/Renderer/RendererDX12.hpp"
	#include "Engine/Renderer/PipelineStateObjectDX12.hpp"
#ifdef ERROR
#undef ERROR
#endif
#else
	#include "Engine/Renderer/RendererDX11.hpp"
#endif // RENDERER_DX12



Rgba8 const DevConsole::ERROR = Rgba8(255, 0, 0, 200);
Rgba8 const DevConsole::WARNING = Rgba8(255, 190, 0, 255);
Rgba8 const DevConsole::INFO_MAJOR = Rgba8(255, 255, 255);
Rgba8 const DevConsole::INFO_MINOR = Rgba8(175, 175, 175);
Rgba8 const DevConsole::SELECTED_LINE = Rgba8(175,255,255);
Rgba8 const DevConsole::VALID_COMMAND = Rgba8(75, 175, 255);
Rgba8 const DevConsole::VALID_REMOTE_COMMAND = Rgba8(150, 175, 255);
Rgba8 const DevConsole::VALID_LINE = Rgba8(150, 255, 150);
Rgba8 const DevConsole::HELP_INFO = Rgba8(150, 175, 220);
Rgba8 const DevConsole::CONSOLE_BACKGROUND = Rgba8(0, 0, 0, 175);
Rgba8 const DevConsole::INPUT_LINE_BACKGROUND = Rgba8(0, 0, 0, 250);
Rgba8 const DevConsole::CHAT = Rgba8(255, 255, 255);
Rgba8 const DevConsole::CHAT_REMOTE = Rgba8(255, 150, 255);

DevConsole::DevConsole(DevConsoleConfig const& config)
	:m_config(config)
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);
	m_config.m_numLinesOnScreen = m_config.m_defaultLinesOnScreen;
}

void DevConsole::Startup()
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	SubscribeEventCallbackObjectMethod("KeyPressed", this, &DevConsole::Event_KeyPressed, false);
	SubscribeEventCallbackObjectMethod("CharPressed", this, &DevConsole::Event_CharInput, false);
	Strings clearArguments;
	clearArguments.push_back("Errors");
	clearArguments.push_back("Screen");
	clearArguments.push_back("Messages");
	clearArguments.push_back("Chat");
	clearArguments.push_back("EventCalls=");
	clearArguments.push_back("NumLines=");
	SubscribeEventCallbackObjectMethod("Clear", clearArguments, this, &DevConsole::Event_CommandClear);
	SubscribeEventCallbackObjectMethod("Help", this, &DevConsole::Event_CommandHelp);
	SubscribeEventCallbackObjectMethod("DevConsoleControls", this, &DevConsole::Event_CommandDevConsoleNavigation);

	Strings showSuggestionsArguments;
	showSuggestionsArguments.push_back("True");
	showSuggestionsArguments.push_back("False");
	SubscribeEventCallbackObjectMethod("ShowSuggestions",showSuggestionsArguments, this, &DevConsole::Event_CommandShowSuggestions);

	Strings resizeConsoleArguments;
	resizeConsoleArguments.push_back("NumLines=");
	resizeConsoleArguments.push_back("Scale=");
	resizeConsoleArguments.push_back("Full");
	resizeConsoleArguments.push_back("Quarter");
	SubscribeEventCallbackObjectMethod("ResizeConsole", resizeConsoleArguments, this, &DevConsole::Event_CommandResizeDevConsole);
	SubscribeEventCallbackObjectMethod("Echo", this, &DevConsole::Event_Echo);

	Strings remoteArgs;
	remoteArgs.push_back("cmd=");
	SubscribeEventCallbackObjectMethod("RemoteCmd", remoteArgs, this, &DevConsole::Event_RemoteCommand);

	Strings notificationArgs;
	notificationArgs.push_back("Hide=All");
	notificationArgs.push_back("Hide=");
	notificationArgs.push_back("Show=All");
	notificationArgs.push_back("Show=Chat");
	notificationArgs.push_back("Show=");
	SubscribeEventCallbackObjectMethod("Notifications", notificationArgs, this, &DevConsole::Event_Notifications);

	AddLine(INFO_MINOR, "Type 'Help' for list of commands", COMMAND_MESSAGE, 1.f);
	m_insertionPointTimer = new Timer(0.5f, &Clock::GetSystemClock());
	m_insertionPointTimer->Start();

	if(m_config.m_activateChatSystem)
	{
		m_chatSystem = new ChatSystem(this);

		Strings chatArgs;
		chatArgs.push_back("Name=");
		chatArgs.push_back("Isolate=True");
		chatArgs.push_back("Isolate=False");
		chatArgs.push_back("Hide=True");
		chatArgs.push_back("Hide=False");
		SubscribeEventCallbackObjectMethod("Chat",chatArgs, this, &DevConsole::Event_Chat);
	}

	SubscribeEventCallbackObjectMethod("PasteRequested", this, &DevConsole::Event_PasteRequested, false);

	Strings xmlArgs;
	xmlArgs.push_back("name=");
	SubscribeEventCallbackObjectMethod("ExecuteXmlCommandScriptFile", xmlArgs, this, &DevConsole::Event_ExecuteXmlCommandScriptFile);

}

void DevConsole::ShutDown()
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);
	UnsubscribeAllEventCallbacksForObject(this);
	delete m_chatSystem;
	m_chatSystem = nullptr;
}

void DevConsole::BeginFrame()
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	if(m_chatSystem)
	{
		m_chatSystem->BeginFrame();
	}

	m_frameNumber++;
	while (m_insertionPointTimer->DecrementPeriodIfElapsed())
	{
		m_insertionPointVisible = !m_insertionPointVisible;
	}

	float deltaSeconds = Clock::GetSystemClock().GetDeltaSeconds();

	if (g_inputSystem)
	{
		float mouseWheelDelta = g_inputSystem->GetWheelDelta();
		if (mouseWheelDelta != 0.f)
		{
			if (mouseWheelDelta > 0.f)
			{
				m_scrollPercentage = GetClamped(m_scrollPercentage + (5.f * deltaSeconds), 0.f, 1.f);
			}

			else if (mouseWheelDelta < 0.f)
			{
				m_scrollPercentage = GetClamped(m_scrollPercentage - (5.f * deltaSeconds), 0.f, 1.f);
			}

			//#TODO have selected line update based on mouse scroll
	// 		const int NUM_LINES_RENDERED = GetClampedInt((int)ceilf(m_config.m_numLinesOnScreen), 0, (int)m_lines.size());
	// 		const int NUM_LINES_BELOW_WINDOW = GetClampedInt((int)floorf((float)m_lines.size() * m_scrollPercentage), 0, (int)m_lines.size() - NUM_LINES_RENDERED);
	// 		int bottomLineNum = GetClampedInt((int)(m_lines.size() - NUM_LINES_RENDERED - 1 - NUM_LINES_BELOW_WINDOW), -1, (int)m_lines.size() - 1);;
	//  		m_selectedLineNum = bottomLineNum;// - 1;
	// 		DebugAddMessage(Stringf("Selected Line: %i", m_selectedLineNum), 5.f);
	// 		DebugAddMessage(Stringf("Bottom Line: %i", bottomLineNum), 5.f, Rgba8::RED, Rgba8::RED);
	// 		if (m_selectedLineNum > bottomLineNum)
	// 		{
	// 		}

		}
	}
}

void DevConsole::EndFrame()
{
	
}

void DevConsole::Render(AABB2 const& screenBounds, Renderer* overrideRenderer) const
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	Renderer* renderer = overrideRenderer ? overrideRenderer : m_config.m_defaultRenderer;
	

	BitmapFont* font = renderer->CreateOrGetBitMapFontFromFile(m_config.m_defaultFontName.c_str());
	if (font == nullptr)
 		return;

	switch (m_mode)
	{
	case HIDDEN:

		renderer->BeginRendererEvent("Draw - DevConsole Alerts");
		Render_Alerts(screenBounds, renderer, *font, m_config.m_defaultFontAspect);
		renderer->EndRendererEvent();
	break;
	case FULL:

		renderer->BeginRendererEvent("Draw - DevConsole");
		Render_Open(screenBounds, renderer, *font, m_config.m_defaultFontAspect);
		renderer->EndRendererEvent();
	break;
	}
	

}

void DevConsole::Render(Camera* camera, Renderer* overrideRenderer) const
{
	Render(AABB2(camera->GetOrthoBottomLeft(), camera->GetOrthoTopRight()), overrideRenderer);
}

//Render
//---------------------------------------------------------------------------------------------------------------------------

void DevConsole::Render_Open(AABB2 const& bounds, Renderer* rendererParent, BitmapFont& font, float fontAspect) const
{
#ifdef RENDERER_DX12
	RendererDX12* renderer = dynamic_cast<RendererDX12*>(rendererParent);
	GUARANTEE_OR_DIE(renderer, "DevConsole trying to render DX12 without DX12 Renderer");
	renderer->BindRootSignature(nullptr);
#else

	RendererDX11* renderer = dynamic_cast<RendererDX11*>(rendererParent);
	GUARANTEE_OR_DIE(renderer, "DevConsole trying to render DX11 without DX11 Renderer");
#endif

	renderer->SetBlendMode(BlendMode::ALPHA);
	renderer->SetDepthMode(DepthMode::DISABLED);
	renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	renderer->BindShader(nullptr);

	Vec2 maxs = bounds.m_maxs * m_consoleWindowScale;
	AABB2 adjustedBounds(bounds.m_mins, maxs);

	int numLines = (int)m_lines.size();
	if (m_showOnlyChat)
	{
		numLines = m_numChatLines; //#TODO may need to add logic for when chat is hidden
	}

	const int NUM_LINES_RENDERED = GetClampedInt((int)ceilf(m_config.m_numLinesOnScreen), 0, numLines);
	const float INPUT_LINE_HEIGHT = (bounds.m_maxs.y - bounds.m_mins.y) / m_config.m_defaultLinesOnScreen;
	const float LINE_HEIGHT = (adjustedBounds.m_maxs.y - (adjustedBounds.m_mins.y + INPUT_LINE_HEIGHT)) / GetClamped(m_config.m_numLinesOnScreen, m_config.m_defaultLinesOnScreen, 100.f);
	const int NUM_LINES_BELOW_WINDOW = GetClampedInt((int)floorf((float)numLines * m_scrollPercentage), 0, numLines - NUM_LINES_RENDERED);

	if (m_config.m_numLinesOnScreen < m_config.m_defaultLinesOnScreen)
	{
		adjustedBounds.m_maxs.y = adjustedBounds.m_mins.y + ((m_config.m_numLinesOnScreen * LINE_HEIGHT) + INPUT_LINE_HEIGHT);
	}

	//Render background
	std::vector<Vertex_PCU> devConsoleVerts;
	AddVertsForAABB2D(devConsoleVerts, adjustedBounds, CONSOLE_BACKGROUND);

	//Render Mouse Wheel bar
	const float BAR_WIDTH = 0.015f * adjustedBounds.GetWidth();
	const float WINDOW_HEIGHT = adjustedBounds.GetHeight() - INPUT_LINE_HEIGHT;
	const float FRAC_LINES_SEEN = (float)NUM_LINES_RENDERED / (float)numLines;
	const float BAR_FRAC_FROM_BOTTOM = (NUM_LINES_BELOW_WINDOW / (float)numLines);// * WINDOW_HEIGHT;
	const float BAR_MINS_Y = adjustedBounds.m_mins.y + (BAR_FRAC_FROM_BOTTOM * WINDOW_HEIGHT) + INPUT_LINE_HEIGHT;
	const float BAR_HEIGHT = GetClamped(FRAC_LINES_SEEN * WINDOW_HEIGHT, 0.1f * WINDOW_HEIGHT, WINDOW_HEIGHT);
	const float BAR_MAXS_Y = BAR_MINS_Y + BAR_HEIGHT;
	AABB2 barBounds(adjustedBounds.m_maxs.x - BAR_WIDTH, BAR_MINS_Y, adjustedBounds.m_maxs.x - (0.005f * adjustedBounds.GetWidth()), BAR_MAXS_Y);
	AddVertsForAABB2D(devConsoleVerts, barBounds, Rgba8(125, 125, 125, 125));

	//Render black bar under input text
	AABB2 inputLineBounds(adjustedBounds.m_mins.x, adjustedBounds.m_mins.y, adjustedBounds.m_maxs.x, adjustedBounds.m_mins.y + INPUT_LINE_HEIGHT);
	AddVertsForAABB2D(devConsoleVerts, inputLineBounds, INPUT_LINE_BACKGROUND);

	renderer->BindTexture(nullptr);
	renderer->DrawVertexArray(devConsoleVerts);

	devConsoleVerts.clear();

	int linesRendered = 0;
	int lineIndexFromEnd = (int)m_lines.size() - 1 - NUM_LINES_BELOW_WINDOW;

	//Render line history
	while (linesRendered < NUM_LINES_RENDERED && lineIndexFromEnd >= 0)
	{
		DevConsoleLine currentLine = m_lines[lineIndexFromEnd]; // starts with last index and moves backwards

		if (m_showOnlyChat && !currentLine.IsChat())
		{
			--lineIndexFromEnd;
			continue;
		}

		else if (m_hideChat && currentLine.IsChat())
		{
			--lineIndexFromEnd;
			continue;
		}

		std::string lineText = currentLine.m_text;

		float lineYPos = adjustedBounds.m_mins.y + (LINE_HEIGHT * linesRendered) + INPUT_LINE_HEIGHT;
		AABB2 lineBounds(adjustedBounds.m_mins.x, lineYPos, adjustedBounds.m_maxs.x, lineYPos + LINE_HEIGHT);

		if (!m_isNavigatingHelpBox && m_selectedLineNum == lineIndexFromEnd)
		{
			//Render higlighted line
			font.AddVertsForTextInBox2D(devConsoleVerts, lineText, lineBounds, LINE_HEIGHT * currentLine.m_textHeightScale * DEFAULT_TEXT_HEIGHT, SELECTED_LINE, fontAspect, Vec2(0.f, 0.5f), SHRINK_TO_FIT);
		}

		else
		{
			font.AddVertsForTextInBox2D(devConsoleVerts, lineText, lineBounds, LINE_HEIGHT * currentLine.m_textHeightScale * DEFAULT_TEXT_HEIGHT, currentLine.m_color, fontAspect, Vec2(0.f, 0.5f), SHRINK_TO_FIT);
		}


		++linesRendered;
		--lineIndexFromEnd;
	}

	//Render input line
	const float CURSOR_WIDTH = INPUT_LINE_HEIGHT * 0.1f;
	float adjustedCellHeight = INPUT_LINE_HEIGHT;
	if (m_inputLine.m_text.size() > 0)
	{
		inputLineBounds.m_maxs.x -= CURSOR_WIDTH; //clamps text length so that cursor is always visible
		adjustedCellHeight = font.AddVertsForTextInBox2D(devConsoleVerts, m_inputLine.m_text, inputLineBounds, INPUT_LINE_HEIGHT * DEFAULT_TEXT_HEIGHT, m_inputLine.m_color, fontAspect, Vec2(0.f, 0.5f), SHRINK_TO_FIT);
	}

	renderer->BindTexture(&font.GetTexture());
	renderer->DrawVertexArray(devConsoleVerts);
	devConsoleVerts.clear();

	//Adds shapes for insertionCursor
	if (m_insertionPointVisible)
	{
		float insertionCursorStart = GetClamped((float)m_insertionPointPosition * adjustedCellHeight * fontAspect, bounds.m_mins.x, bounds.m_maxs.x - CURSOR_WIDTH);
		float insertionCursorEnd = GetClamped(insertionCursorStart + CURSOR_WIDTH, bounds.m_mins.x + CURSOR_WIDTH, bounds.m_maxs.x);
		AABB2 insertionCursorBounds(insertionCursorStart, bounds.m_mins.y, insertionCursorEnd, bounds.m_mins.y + INPUT_LINE_HEIGHT);
		AddVertsForAABB2D(devConsoleVerts, insertionCursorBounds, INFO_MAJOR);
	}

	//Adds frame to devConsole to cover the half line overlap of top text
	if (m_consoleWindowScale < 1.f || m_config.m_numLinesOnScreen < m_config.m_defaultLinesOnScreen)
	{
		AddVertsForAABB2DFrame(devConsoleVerts, adjustedBounds, LINE_HEIGHT * 0.5f, Rgba8::GREY);
	}

	if (devConsoleVerts.size() > 0)
	{
		renderer->BindTexture(nullptr);
		renderer->DrawVertexArray(devConsoleVerts);
	}

	//render help box
	if (m_showSuggestions && m_helpBox.m_shouldRender)
	{
		const int NUM_LINES = (int)m_helpBox.m_text.size();
		const float TEXT_HEIGHT = DEFAULT_TEXT_HEIGHT;
		const float HELP_BOX_WIDTH = BitmapFont::GetTextWidth(INPUT_LINE_HEIGHT * TEXT_HEIGHT, m_helpBox.m_longestLine, fontAspect);

		float boxStartPos = GetClamped((float)m_helpBox.m_startIndex * adjustedCellHeight * fontAspect, bounds.m_mins.x, bounds.m_maxs.x - HELP_BOX_WIDTH);
		AABB2 subBox(boxStartPos, bounds.m_mins.y + INPUT_LINE_HEIGHT, boxStartPos + HELP_BOX_WIDTH, bounds.m_mins.y + (INPUT_LINE_HEIGHT * (float)NUM_LINES) + INPUT_LINE_HEIGHT);
		AddVertsForAABB2D(devConsoleVerts, subBox, INPUT_LINE_BACKGROUND);

		renderer->BindTexture(nullptr);
		renderer->DrawVertexArray(devConsoleVerts);
		devConsoleVerts.clear();

		for (int lineNum = 0; lineNum < NUM_LINES; ++lineNum)
		{
			float lineYPos = subBox.m_mins.y + (INPUT_LINE_HEIGHT * lineNum);
			AABB2 lineBounds(subBox.m_mins.x, lineYPos, subBox.m_maxs.x, lineYPos + INPUT_LINE_HEIGHT);

			if (m_isNavigatingHelpBox && lineNum == m_helpBoxSelectedLinePosition)
			{
				//Render highlighted help text
				font.AddVertsForTextInBox2D(devConsoleVerts, m_helpBox.m_text[lineNum], lineBounds, INPUT_LINE_HEIGHT * TEXT_HEIGHT, SELECTED_LINE, fontAspect, Vec2(0.f, 0.5f), SHRINK_TO_FIT);
				continue;
			}

			font.AddVertsForTextInBox2D(devConsoleVerts, m_helpBox.m_text[lineNum], lineBounds, INPUT_LINE_HEIGHT * TEXT_HEIGHT, INFO_MINOR, fontAspect, Vec2(0.f, 0.5f), SHRINK_TO_FIT);
		}

		renderer->BindTexture(&font.GetTexture());
		renderer->DrawVertexArray(devConsoleVerts);
		devConsoleVerts.clear();
	}
}

void DevConsole::Render_Alerts(AABB2 const& bounds, Renderer* rendererParent, BitmapFont& font, float fontAspect) const
{
#ifdef RENDERER_DX12
	RendererDX12* renderer = dynamic_cast<RendererDX12*>(rendererParent);
	GUARANTEE_OR_DIE(renderer, "DevConsole trying to render DX12 without DX12 Renderer");
	renderer->BindRootSignature(nullptr);
#else

	RendererDX11* renderer = dynamic_cast<RendererDX11*>(rendererParent);
	GUARANTEE_OR_DIE(renderer, "DevConsole trying to render DX11 without DX11 Renderer");
#endif

	renderer->SetBlendMode(BlendMode::ALPHA);
	renderer->SetDepthMode(DepthMode::DISABLED);
	renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	renderer->BindShader(nullptr);

	constexpr int NUM_LINES_RENDERED = 10;
	AABB2 windowBounds = bounds.GetBoxAtUVS(AABB2(0.5f, 0.f, .99f, 0.2f));
	const int LINE_HEIGHT = (int)(windowBounds.GetHeight() / NUM_LINES_RENDERED);
	Verts verts;
	const double CURRENT_TIME = GetCurrentTimeSeconds();
	constexpr double TIME_VISIBLE = 5.0;

	int linesRendered = 0;
	int lineIndex = (int)m_lines.size() - 1;

	//Render line history
	while (linesRendered < NUM_LINES_RENDERED && lineIndex >= 0)
	{

		DevConsoleLine currentLine = m_lines[lineIndex]; // starts with last index and moves backwards
		const double TIME_EXISTED = CURRENT_TIME - currentLine.m_timePrinted;
		if (TIME_EXISTED > TIME_VISIBLE)
		{
			--lineIndex;
			continue;
		}

		if (!ShowAsAlert(currentLine))
		{
			--lineIndex;
			continue;
		}

		float alpha = GetFractionWithinRange((float)TIME_EXISTED, (float)TIME_VISIBLE, 0.f);
		Rgba8 color = currentLine.m_color;
		color.a = DenormalizeByte(alpha);

		std::string lineText = currentLine.m_text;

		float lineYPos = windowBounds.m_mins.y + (LINE_HEIGHT * linesRendered) + LINE_HEIGHT;
		AABB2 lineBounds(windowBounds.m_mins.x, lineYPos, windowBounds.m_maxs.x, lineYPos + LINE_HEIGHT);

		font.AddVertsForTextInBox2D(verts, lineText, lineBounds, LINE_HEIGHT * DEFAULT_TEXT_HEIGHT, color, fontAspect, Vec2(1.f, 0.5f), SHRINK_TO_FIT);

		++linesRendered;
		--lineIndex;
	}

	if (verts.size() > 0)
	{
		renderer->BindTexture(&font.GetTexture());
		renderer->DrawVertexArray(verts);
	}
}

void DevConsole::Execute(std::string const& consoleCommandText)
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	Strings linesToExecute = SplitStringOnDelimiter(consoleCommandText, '\n');
	
	for (int lineNum = 0; lineNum < (int)linesToExecute.size(); ++lineNum)
	{
		Strings words = SplitStringOnDelimiter(linesToExecute[lineNum], ' ', '\"');
		if(words.empty())
			continue;

		std::string firstWordLowercase = GetLowercase(words[0]);

		//For Echo Command parse entire line as a message to be sent
		if(firstWordLowercase == "echo")
		{
			std::string message;
			for(int i = 1; i < (int)words.size(); ++i)
			{
				if(i == 1)
				{
					message.append(words[i]);
					continue;
				}

				message.append(Stringf(" %s", words[i].c_str()));
			}

			EventArgs echoArgs;
			echoArgs.SetValue("message", message);
			FireEvent(words[0], echoArgs);
			return;
		}

		//Keep chat messages from trying to execute, they are handled internally by ChatSystem
		char firstChar = words[0][0];
		if(firstChar == ChatSystem::CHAT_MESSAGE_MARKER)
		{
			RemoveLastLine();
			return;
		}
		

		EventArgs args;

		//skip index 0 because words[0] is the name of the event, everything else is either key or value of argument
		for (int argNum = 1; argNum < (int)words.size(); ++argNum)
		{
			Strings keyValuePairs = SplitStringOnDelimiter(words[argNum], '=');

			//Avoid firing event on empty string
			if (keyValuePairs.size() < 1)
				continue;
			
			//Useful for cases like "True/False"
			if (keyValuePairs.size() <= 1)
			{
				args.SetValue(keyValuePairs[0].c_str(), "");
				continue;
			}

			args.SetValue(keyValuePairs[0].c_str(), keyValuePairs[1]);
		}

		//handle sending command over network
		if(firstWordLowercase == "remotecmd")
		{
			if (g_networkSystem && g_networkSystem->IsConnected())
			{
				std::string command = args.GetValue("cmd", "NO_COMMAND");
				if (command != "NO_COMMAND")
				{
					std::string commandLowerCase = GetLowercase(command);
					std::string commandWithCMDLowercase = "cmd=";
					commandWithCMDLowercase.append(commandLowerCase);
					std::string completeLine = command;
					for (int i = 1; i < (int)words.size(); ++i)
					{
						std::string lowerCaseWord = GetLowercase(words[i]);
						if (lowerCaseWord == commandWithCMDLowercase)
							continue;

						completeLine.append(Stringf(" %s", lowerCaseWord.c_str()).c_str());
					}

					g_networkSystem->Send(completeLine);
					return;
				}

				else
				{
					m_lines[(int)m_lines.size() - 1].m_color = WARNING;
					AddLine(DevConsole::WARNING, Stringf("Missing '%s' argument for networked command: %s", "cmd=", consoleCommandText.c_str()),COMMAND_MESSAGE, 0.65f);
					return;
				}
			}

			else
			{
				m_lines[(int)m_lines.size() - 1].m_color = WARNING;
				AddLine(DevConsole::WARNING, "Trying to send RemoteCmd without a connected network system",COMMAND_MESSAGE, 0.65f);
			}
		}

		//If event was invalid, or fired but had no subscribers, message to dev console
		if (!FireEvent(words[0], args))
		{
			m_lines[(int)m_lines.size() - 1].m_color = WARNING;
			std::string failureMessage = "Unknown Command: ";
			Rgba8 failureColor = ERROR;

			if (g_eventSystem->IsValidEvent(words[0]))
			{
				failureMessage = "No subscribers to: ";
				failureColor = WARNING;
			}

			AddLine(failureColor, failureMessage.append(words[0]), COMMAND_MESSAGE, 0.65f);
		}
	}
}

DevConsoleMode DevConsole::GetMode() const
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	return m_mode;
}

void DevConsole::SetMode(DevConsoleMode mode)
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	m_mode = mode;
}

void DevConsole::ToggleMode(DevConsoleMode mode)
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	if (m_mode == mode)
	{
		m_mode = HIDDEN;
		return;
	}

	m_mode = mode;
	m_scrollPercentage = 0.f;
}

bool DevConsole::IsOpen() const
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	return m_mode == FULL;
}

void DevConsole::AddLine(Rgba8 const& color, std::string const& text, float heightPercent, bool isMessage)
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	DevConsoleLine newLine;
	newLine.m_color = color;
	newLine.m_text = text;
	newLine.m_timePrinted = GetCurrentTimeSeconds();
	newLine.m_frameNumberPrinted = m_frameNumber;
	newLine.m_textHeightScale = heightPercent;
	if(isMessage)
	{
		newLine.m_lineType = COMMAND_MESSAGE;
	}
	m_lines.push_back(newLine);
}

void DevConsole::AddLine(Rgba8 const& color, std::string const& text, DevConsoleLineType lineType, float heightPercent)
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	DevConsoleLine newLine;
	newLine.m_color = color;
	newLine.m_text = text;
	newLine.m_timePrinted = GetCurrentTimeSeconds();
	newLine.m_frameNumberPrinted = m_frameNumber;
	newLine.m_textHeightScale = heightPercent;
	newLine.m_lineType = lineType;
	m_lines.push_back(newLine);

	if (lineType == CHAT_MESSAGE || lineType == CHAT_MESSAGE_REMOTE)
	{
		m_numChatLines++;
	}
}

void DevConsole::AddLine(DevConsoleLineType lineType, std::string const& text, Rgba8 const& color, float overrideHeightPercent)
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	DevConsoleLine newLine;
	newLine.m_text = text;
	newLine.m_timePrinted = GetCurrentTimeSeconds();
	newLine.m_frameNumberPrinted = m_frameNumber;
	newLine.m_lineType = lineType;
	switch (lineType)
	{
	case COMMAND:
		newLine.m_color = DevConsole::VALID_COMMAND;
		newLine.m_textHeightScale = 1.f;
		break;
	case COPYABLE_MESSAGE:
		newLine.m_color = DevConsole::HELP_INFO;
		newLine.m_textHeightScale = .75f;
		break;
	case COMMAND_MESSAGE:
		newLine.m_color = DevConsole::WARNING;
		newLine.m_textHeightScale = .65f;
		break;
	case CHAT_MESSAGE:
		newLine.m_color = DevConsole::CHAT;
		newLine.m_textHeightScale = 1.f;
		m_numChatLines++;
		break;
	case CHAT_MESSAGE_REMOTE:
		newLine.m_color = DevConsole::CHAT_REMOTE;
		newLine.m_textHeightScale = 1.f;
		m_numChatLines++;
		break;
	case ALERT_MESSAGE:
		newLine.m_color = DevConsole::WARNING;
		newLine.m_textHeightScale = .65f;
		break;
	default:
		break;
	}

	if(color != Rgba8::BLACK)
	{
		newLine.m_color = color;
	}

	if(overrideHeightPercent > 0)
	{
		newLine.m_textHeightScale = overrideHeightPercent;
	}

	m_lines.push_back(newLine);
}

void DevConsole::AddLineAndExecute(DevConsoleLineType lineType, std::string const& text, Rgba8 const& color, float overrideHeightPercent)
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	AddLine(lineType, text, color, overrideHeightPercent);
	Execute(GetTextFromMostRecentLine());
}

void DevConsole::EditInputText(unsigned char keyCode)
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	if (keyCode == KEYCODE_BACKSPACE && m_inputLine.m_text.size() > 0 && m_insertionPointPosition > 0)
	{
		m_inputLine.m_text.erase(m_insertionPointPosition - 1, 1);
		m_insertionPointPosition--;
		m_insertionPointPosition = GetClampedInt(m_insertionPointPosition, 0, (int)m_inputLine.m_text.size());
		m_isNavigatingHelpBox = false;
	}

	else if (keyCode >= 32 && keyCode <= 126 && keyCode != '~' && keyCode != '`')
	{
		std::string stringToInsert(1, (char)keyCode);
		m_inputLine.m_text.insert(m_insertionPointPosition, stringToInsert);
		m_insertionPointPosition++;
		m_isNavigatingHelpBox = false;
	}
	

	UpdateInputLineColor();
	ResetInsertionPointBlink();
}

void DevConsole::AppendToInputText(std::string const& textToAdd)
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);
	m_inputLine.m_text.insert(m_insertionPointPosition, textToAdd);
	m_insertionPointPosition += (int)textToAdd.size();
	m_isNavigatingHelpBox;
	UpdateInputLineColor();
	ResetInsertionPointBlink();
}

void DevConsole::DeleteFromInputText(unsigned char keyCode)
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	if (keyCode == KEYCODE_DELETE && m_inputLine.m_text.size() > 0)
	{
		m_scrollPercentage = 0.f;
		m_inputLine.m_text.erase(m_insertionPointPosition, 1);
		m_insertionPointPosition = GetClampedInt(m_insertionPointPosition, 0, (int)m_inputLine.m_text.size());
		ResetInsertionPointBlink();
		UpdateInputLineColor();
	}
}

void DevConsole::MoveInsertionCursor(unsigned char keyCode)
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	if (keyCode == KEYCODE_LEFTARROW)
	{
		m_insertionPointPosition--;
		m_insertionPointPosition = GetClampedInt(m_insertionPointPosition, 0, (int)m_inputLine.m_text.size());
	}

	if (keyCode == KEYCODE_RIGHTARROW)
	{
		m_insertionPointPosition++;
		m_insertionPointPosition = GetClampedInt(m_insertionPointPosition, 0, (int)m_inputLine.m_text.size());
	}

	if (keyCode == KEYCODE_END)
	{
		m_insertionPointPosition = (int)m_inputLine.m_text.size();
	}

	if (keyCode == KEYCODE_HOME)
	{
		m_insertionPointPosition = 0;
	}

	ResetInsertionPointBlink();
	UpdateInputLineColor();
}

void DevConsole::NavigateLines(unsigned char keyCode)
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	if (keyCode == KEYCODE_TAB && m_helpBox.m_shouldRender && m_showSuggestions)
	{
		Strings inputStrings = SplitStringOnDelimiter(m_inputLine.m_text, ' ', false);
		if (!m_isNavigatingHelpBox)
		{
			m_isNavigatingHelpBox = true;
			m_helpBoxSelectedLinePosition = 0;
			std::string argument = m_helpBox.m_text[0];
			argument.pop_back();

			int lastWhiteSpace = GetIndexOfLastChar(m_inputLine.m_text, ' ');
			if (lastWhiteSpace == (int)m_inputLine.m_text.length() - 1)
			{
				argument.erase(0,1);
				m_inputLine.m_text += argument;
			}

			else
			{
				m_inputLine.m_text = inputStrings[0];
				const int NUM_STRINGS = (int)inputStrings.size();

				if (NUM_STRINGS > 2)
				{
					m_inputLine.m_text += " ";
				}

				for (int i = 1; i < NUM_STRINGS - 1; ++i)
				{
					m_inputLine.m_text += inputStrings[i];
					if (i < NUM_STRINGS - 2)
					{
						m_inputLine.m_text += " ";
					}
				}

				m_inputLine.m_text += argument;
			}

			
			m_insertionPointPosition = (int)m_inputLine.m_text.size();
			ResetInsertionPointBlink();
			return;
		}

		m_inputLine.m_text.clear();
		for (int i = 0; i < (int)inputStrings.size() - 1; ++i)
		{
			m_inputLine.m_text += inputStrings[i];
			if (i < (int)inputStrings.size() - 2)
			{
				m_inputLine.m_text += " ";
			}
		}

		m_isNavigatingHelpBox = false;
		m_insertionPointPosition = (int)m_inputLine.m_text.size();
		ResetInsertionPointBlink();
		return;
	}

	if (keyCode == KEYCODE_UPARROW)
	{
		if (m_helpBox.m_shouldRender && m_isNavigatingHelpBox)
		{
			m_helpBoxSelectedLinePosition++;
			if (m_helpBoxSelectedLinePosition >= (int)m_helpBox.m_text.size())
			{
				m_helpBoxSelectedLinePosition = 0;
			}

			Strings inputStrings = SplitStringOnDelimiter(m_inputLine.m_text, ' ');
			m_inputLine.m_text.clear();
			std::string argument = m_helpBox.m_text[m_helpBoxSelectedLinePosition];
			argument.pop_back();
			for (int i = 0; i < (int)inputStrings.size() - 1; ++i)
			{
				m_inputLine.m_text += inputStrings[i];
				if (i < (int)inputStrings.size() - 2)
				{
					m_inputLine.m_text += " ";
				}
			}

			m_inputLine.m_text = m_inputLine.m_text + argument;
			m_insertionPointPosition = (int)m_inputLine.m_text.size();
			ResetInsertionPointBlink();
			return;
		}

		const int NUM_LINES = (int)m_lines.size();
		if (NUM_LINES <= 0)
			return;

		int numMessageLines = 0;
		m_selectedLineNum--;

		//Skip over error messages
		for (int lineNum = 0; lineNum < NUM_LINES; ++lineNum)
		{
			if (m_selectedLineNum < 0)
			{
				m_selectedLineNum = NUM_LINES - 1;
			}

			if (m_selectedLineNum >= NUM_LINES)
			{
				m_selectedLineNum = 0;
			}

			DevConsoleLine highlightedLine = m_lines[m_selectedLineNum];
			if (!highlightedLine.IsCopyable())
			{
				numMessageLines++;
				m_selectedLineNum--;
				if (numMessageLines >= NUM_LINES) //returns from function if there are no valid lines to copy
				{
					m_selectedLineNum = -1;
					return;
				}
				continue;
			}

			break;
		}
			
		m_inputLine.m_text = m_lines[m_selectedLineNum].m_text;
		m_insertionPointPosition = (int)m_inputLine.m_text.size();
	}

	if (keyCode == KEYCODE_DOWNARROW)
	{
		if (m_helpBox.m_shouldRender && m_isNavigatingHelpBox)
		{
			m_helpBoxSelectedLinePosition--;
			if (m_helpBoxSelectedLinePosition < 0)
			{
				m_helpBoxSelectedLinePosition = (int)m_helpBox.m_text.size() - 1;
			}

			Strings inputStrings = SplitStringOnDelimiter(m_inputLine.m_text, ' ');
			m_inputLine.m_text.clear();
			std::string argument = m_helpBox.m_text[m_helpBoxSelectedLinePosition];
			argument.pop_back();
			for (int i = 0; i < (int)inputStrings.size() - 1; ++i)
			{
				m_inputLine.m_text += inputStrings[i];
				if (i < (int)inputStrings.size() - 2)
				{
					m_inputLine.m_text += " ";
				}
			}

			m_inputLine.m_text = m_inputLine.m_text + argument;
			m_insertionPointPosition = (int)m_inputLine.m_text.size();
			ResetInsertionPointBlink();
			return;
		}

		const int NUM_LINES = (int)m_lines.size();
		if (NUM_LINES <= 0)
			return;

		int numMessageLines = 0;
		m_selectedLineNum++;

		//Skip over error messages
		for (int lineNum = 0; lineNum < NUM_LINES; ++lineNum)
		{
			if (m_selectedLineNum < 0)
			{
				m_selectedLineNum = NUM_LINES - 1;
			}
			if (m_selectedLineNum >= NUM_LINES)
			{
				m_selectedLineNum = 0;
			}

			DevConsoleLine highlighedLine = m_lines[m_selectedLineNum];
			if (!highlighedLine.IsCopyable())
			{
				m_selectedLineNum++;
				numMessageLines++;
				if (numMessageLines >= NUM_LINES) //returns from function if there are no valid lines to copy
				{
					m_selectedLineNum = -1;
					return;
				}
				continue;
			}

			break;
		}

		m_inputLine.m_text = m_lines[m_selectedLineNum].m_text;
		m_insertionPointPosition = (int)m_inputLine.m_text.size();
	}


	ResetInsertionPointBlink();
	UpdateInputLineColor();
}

bool DevConsole::ExecuteInputLine()
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	if (m_inputLine.m_text.size() > 0)
	{
		char firstChar = m_inputLine.m_text[0];
		if (firstChar == ChatSystem::CHAT_MESSAGE_MARKER)
		{
			if(m_chatSystem)
			{
				m_chatSystem->SubmitChatMessage(m_inputLine.m_text);
			}
			ClearInputLine();
			EditInputText(ChatSystem::CHAT_MESSAGE_MARKER);
		}

		else
		{
			AddLine(VALID_COMMAND, m_inputLine.m_text);
			Execute(GetTextFromMostRecentLine());
			ClearInputLine();
		}

		m_scrollPercentage = 0.f;
		return true;
	}
	
	ClearInputLine();
	return false;
}

void DevConsole::ClearCommands(EventArgs& args)
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	bool completeClear = true;
	if (args.HasKey("Screen"))
	{
		int numLinesOnScreen = (int)floorf(m_config.m_numLinesOnScreen);
		int totalLines = (int)m_lines.size() - 1;
		int indexOfFirstLineToDelete = GetClampedInt(totalLines - numLinesOnScreen, 0, totalLines);
		m_lines.erase(m_lines.begin() + GetClampedInt(indexOfFirstLineToDelete, 1, totalLines), m_lines.end() - 1);
		completeClear = false;

		//#TODO: handle when screen is different because of mouse scroll
	}

	if(args.HasKey("Messages"))
	{
		completeClear = false;
		for (int lineNum = 1; lineNum < (int)m_lines.size(); ++lineNum)
		{
			if (!m_lines[lineNum].IsCopyable())
			{
				m_lines.erase(m_lines.begin() + lineNum);
				lineNum--;
			}
		}
	}

	if (args.HasKey("Errors"))
	{
		completeClear = false;
		for (int lineNum = 0; lineNum < (int)m_lines.size(); ++lineNum)
		{
			if ((!g_eventSystem->IsValidEvent(m_lines[lineNum].m_text) && m_lines[lineNum].IsCopyable()) || m_lines[lineNum].m_color == ERROR || m_lines[lineNum].m_color == WARNING)
			{
				m_lines.erase(m_lines.begin() + lineNum);
				lineNum--;
			}
		}
	}

	if (args.HasKey("EventCalls"))
	{
		std::string eventName = args.GetValue("EventCalls", "Invalid");
		eventName = GetLowercase(eventName);
		if (eventName != "Invalid")
		{
			completeClear = false;
			int numLinesCleared = 0;
			for (int lineNum = 0; lineNum < (int)m_lines.size() - 1; ++lineNum)
			{
				DevConsoleLine currentLine = m_lines[lineNum];
				Strings splitText = SplitStringOnDelimiter(currentLine.m_text, ' ');

				if (currentLine.m_color == HELP_INFO)
					continue;

				if (splitText.size() > 0 && GetLowercase(splitText[0]) == eventName)
				{
					m_lines.erase(m_lines.begin() + lineNum);
					lineNum--;
					numLinesCleared++;
				}
			}

			if (!args.HasKey("HideClearMessage"))
			{
				if (numLinesCleared < 1)
				{
					AddLine(WARNING, Stringf(" %i lines cleared - check spelling or capitalization", numLinesCleared),COMMAND_MESSAGE, 0.75f);
					return;
				}

				AddLine(INFO_MINOR, Stringf(" %i lines cleared", numLinesCleared),COMMAND_MESSAGE, 0.75f);
			}
		}
	}

	int numLines = args.GetValue("NumLines", -1);
	if (numLines >= 0)
	{
		completeClear = false;
		int totalLines = (int)m_lines.size() - 1;
		int indexOfFirstLineToDelete = GetClampedInt(totalLines - GetClampedInt(numLines, 0, totalLines), 1, totalLines);
		m_lines.erase(m_lines.begin() + indexOfFirstLineToDelete, m_lines.end() - 1);
	}

	else if (args.HasKey("NumLines"))
	{
		completeClear = false;
		m_lines[(int)m_lines.size() - 1].m_color = WARNING;
		AddLine(ERROR, "Invalid Number", COMMAND_MESSAGE, 0.75f);
	}

	else if (args.HasKey("Chat"))
	{
		for(int i =0; i < (int)m_lines.size(); ++i)
		{
			DevConsoleLine currentLine = m_lines[i];
			if(currentLine.m_lineType == CHAT_MESSAGE || currentLine.m_lineType == CHAT_MESSAGE_REMOTE)
			{
				m_lines.erase(m_lines.begin() + i);
				i--;
			}
		}
	}

	else if (completeClear)
	{
		m_lines.erase(m_lines.begin() + 1, m_lines.end()); // does not clear first line which gives help instructions
		m_numChatLines = 0;
	}

}

void DevConsole::HelpCommand()
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	AddLine(INFO_MINOR, "---Registered Commands---", COMMAND_MESSAGE, 1.f);

	std::vector<EventInfo> eventInfos = g_eventSystem->GetAllRegisteredEventInfos();

	std::sort(eventInfos.begin(), eventInfos.end(),
		[](EventInfo const& a, EventInfo const& b)
		{
			return a.m_name.GetOriginalString() < b.m_name.GetOriginalString();
		}
	);

	for (int eventNum = 0; eventNum < (int)eventInfos.size(); ++eventNum)
	{
		if (!eventInfos[eventNum].m_visibleEvent)
			continue;

		std::string eventName = eventInfos[eventNum].m_name.GetOriginalString();
		AddLine(HELP_INFO, eventName, COPYABLE_MESSAGE, 0.75f);
	}

	AddLine(INFO_MINOR, "-------------------------", COMMAND_MESSAGE, 1.f);
}

void DevConsole::ClearInputLine()
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	if (m_inputLine.m_text.size() <= 0)
	{
		SetMode(HIDDEN);
		return;
	}

	m_inputLine.m_text.clear();
	m_insertionPointPosition = 0;
	m_isNavigatingHelpBox = false;
	m_helpBox.m_shouldRender = false;
}

void DevConsole::ResizeConsoleWindow(EventArgs& args)
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	if (args.HasKey("NumLines"))
	{
		float numLines = GetClamped(args.GetValue("NumLines", m_config.m_defaultLinesOnScreen), 1.f, 100.f);
		numLines = floorf(numLines) + 0.5f;
		m_config.m_numLinesOnScreen = numLines;
		return;
	}

	if (args.HasKey("Scale"))
	{
		float windowScale = args.GetValue("Scale", 1.f);
		if (windowScale < 0.25f)
		{
			AddLine(WARNING, "Warning: Too small value to scale console window",COMMAND_MESSAGE, 0.75f);
			return;
		}

		m_consoleWindowScale = GetClamped(windowScale, 0.25f, 1.f);
		return;
	}

	if (args.HasKey("Quarter"))
	{
		m_consoleWindowScale = 0.5f;
		m_config.m_numLinesOnScreen = m_config.m_defaultLinesOnScreen;
		return;
	}

	if (args.HasKey("Full"))
	{
		m_consoleWindowScale = 1.f;
		m_config.m_numLinesOnScreen = m_config.m_defaultLinesOnScreen;
		return;
	}

	m_consoleWindowScale = 1.f;
	m_config.m_numLinesOnScreen = m_config.m_defaultLinesOnScreen;
	return;
}

void DevConsole::ResetInsertionPointBlink()
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	m_insertionPointTimer->Start();
	m_insertionPointVisible = true;
}

void DevConsole::UpdateInputLineColor()
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	Strings formattedtext = SplitStringOnDelimiter(m_inputLine.m_text.c_str(), ' ');

	if (formattedtext.size() > 0 && g_eventSystem->IsValidEvent(formattedtext[0]))
	{
		m_inputLine.m_color = VALID_LINE;
		FormatHelpBoxForEventName(formattedtext[0]);
	}

	else
	{
		m_inputLine.m_color = INFO_MINOR;
		m_helpBox.m_shouldRender = false;
	}
}

void DevConsole::SaveHelpTextForEventNames()
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	HashedStrings eventNames = g_eventSystem->GetAllRegisteredEventNames();

	for (int eventNum = 0; eventNum < (int)eventNames.size(); ++eventNum)
	{
		HashedCaseInsensitiveString eventName = eventNames[eventNum];
		Strings helpTexts;
		if (g_eventSystem->GetArgumentFormatsForEventName(eventName, helpTexts))
		{
			auto found = m_helpTextByEventName.find(eventName);
			if (found == m_helpTextByEventName.end())
			{
				m_helpTextByEventName.insert({ eventName, helpTexts });
				continue;
			}
			
			found->second = helpTexts;
		}
	}
}

void DevConsole::FormatHelpBoxForEventName(std::string const& eventName)
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	auto found = m_helpTextByEventName.find(GetLowercase(eventName));
	if (found == m_helpTextByEventName.end())
	{
		m_helpBox.m_shouldRender = false;
		return;
	}

	m_helpBox.m_shouldRender = true;
	m_helpBox.m_startIndex = GetClampedInt(GetIndexOfLastChar(m_inputLine.m_text, ' '), (int)eventName.length(), 99999);
	m_helpBox.m_text = found->second;
	m_helpBox.m_longestLine.clear();
	for (int stringNum = 0; stringNum < (int)m_helpBox.m_text.size(); ++stringNum)
	{
		if (m_helpBox.m_text[stringNum].length() > m_helpBox.m_longestLine.length())
		{
			m_helpBox.m_longestLine = m_helpBox.m_text[stringNum];
		}
	}
}

void DevConsole::ShowSuggestions(bool show)
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	m_showSuggestions = show;
	if (!m_showSuggestions)
	{
		m_helpBox.m_shouldRender = false;
		m_helpBoxSelectedLinePosition = 0;
		m_isNavigatingHelpBox = false;
	}
}

bool DevConsole::ShowAsAlert(DevConsoleLine const& line) const
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	return m_visibleAlerts[line.m_lineType];
}

void DevConsole::SanitizePastedText(std::string& pastedText) const
{
	//Removes new lines in pasted text
	for (size_t characterIndex = 0; characterIndex < pastedText.size(); ++characterIndex)
	{
		char& currentCharacter = pastedText[characterIndex];

		if (currentCharacter == '\r' || currentCharacter == '\n')
		{
			currentCharacter = ' ';
		}
	}
}

DevConsoleLineType DevConsole::GetLineTypeFromNotificationString(std::string const& string)
{
	if(string == "chat")
		return CHAT_MESSAGE_REMOTE;
	if(string == "commands")
		return COMMAND_REMOTE;
	if(string == "alerts")
		return ALERT_MESSAGE;

	return INVALID_LINE_TYPE;
}

void DevConsole::ExecuteXmlCommandScriptFile(std::string const& commandScriptXmlFilePathName)
{
	XmlDocument xmlDoc;
	XmlResult result = xmlDoc.LoadFile(commandScriptXmlFilePathName.c_str());
	if (result != tinyxml2::XML_SUCCESS)
	{
		AddLine(DevConsoleLineType::ALERT_MESSAGE, Stringf("Failed to load XML File: %s", commandScriptXmlFilePathName.c_str()));
		return;
	}

	XmlElement* rootElement = xmlDoc.RootElement();
	if (!rootElement)
	{
		AddLine(DevConsoleLineType::ALERT_MESSAGE, Stringf("Failed to load root element for XML File: %s", commandScriptXmlFilePathName.c_str()));
		return;
	}

	ExecuteXmlCommandScriptNode(*rootElement);
}

void DevConsole::ExecuteXmlCommandScriptNode(XmlElement const& commandScriptXmlElement)
{
	XmlElement const* childElement = commandScriptXmlElement.FirstChildElement();
	while (childElement)
	{
		std::string elementName = childElement->Name();
		Strings attributeArgs;
		XmlAttribute const* attribute = childElement->FirstAttribute();
		while (attribute)
		{
			std::string attributeArg = Stringf("%s=\"%s\" ", attribute->Name(), attribute->Value());
			attributeArgs.push_back(attributeArg);
			attribute = attribute->Next();
		}

		std::string lineArg = elementName + " ";
		for (int i = 0; i < (int)attributeArgs.size(); ++i)
		{
			lineArg += attributeArgs[i];
		}

		AddLineAndExecute(DevConsoleLineType::COMMAND, lineArg);

		childElement = childElement->NextSiblingElement();

	}
}

void DevConsole::RemoveLastLine()
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	m_lines.pop_back();
}

std::string DevConsole::GetTextFromMostRecentLine()
{
	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	int numLines = (int)(m_lines.size());
	if (numLines <= 0)
	{
		return "";
	}

	int lastIndex = numLines - 1;
	return m_lines[lastIndex].m_text;
}

//Events
//---------------------------------------------------------------------

bool DevConsole::Event_KeyPressed(EventArgs& args)
{
	std::scoped_lock<std::recursive_mutex> lock(g_devConsole->m_devConsoleMutex);

	unsigned char keyCode = args.GetValue("KeyCode", (unsigned char)1);
	if (keyCode == KEYCODE_TILDE)
	{
		if (m_inputLine.m_text.size() <= 1 && m_inputLine.m_text[0] == ChatSystem::CHAT_MESSAGE_MARKER)
		{
			m_inputLine.m_text.clear();
			m_insertionPointPosition = 0;
		}

		ToggleMode(FULL);
		m_selectedLineNum = -1;

		if (m_mode == FULL)
		{
			SaveHelpTextForEventNames();
		}

		if (g_inputSystem && g_inputSystem->GetCursorMode() != CursorMode::POINTER)
		{
			g_inputSystem->SetCursorMode(CursorMode::POINTER);
		}

		return true;
	}

	if (m_mode == HIDDEN)
		return false;

	if (keyCode == KEYCODE_LEFTARROW || keyCode == KEYCODE_RIGHTARROW || keyCode == KEYCODE_HOME || keyCode == KEYCODE_END)
	{
		MoveInsertionCursor(keyCode);
		return true;
	}

	if (keyCode == KEYCODE_TAB || keyCode == KEYCODE_UPARROW || keyCode == KEYCODE_DOWNARROW)
	{
		NavigateLines(keyCode);
		return true;
	}

	if (!g_devConsole->m_isNavigatingHelpBox)
	{
		m_selectedLineNum = -1; //reset selected line
	}

	if (keyCode == KEYCODE_DELETE)
	{
		DeleteFromInputText(keyCode);
		return true;
	}

	if (keyCode == KEYCODE_RETURN)
	{
		ExecuteInputLine();
		return true;
	}

	if (keyCode == KEYCODE_ESC)
	{
		ClearInputLine();
		return true;
	}

	return true;
}

bool DevConsole::Event_CharInput(EventArgs& args)
{
	std::scoped_lock<std::recursive_mutex> lock(g_devConsole->m_devConsoleMutex);

	unsigned char keyCode = args.GetValue("KeyCode", (unsigned char)1);
	if (GetMode() == HIDDEN)
	{
		if (m_chatSystem && (keyCode == 't' || keyCode == 'T'))
		{
			SetMode(FULL);
			m_selectedLineNum = -1;
			SaveHelpTextForEventNames();
			m_insertionPointPosition = 0;
			m_inputLine.m_text.clear();
			

			if (g_inputSystem && g_inputSystem->GetCursorMode() != CursorMode::POINTER)
			{
				g_inputSystem->SetCursorMode(CursorMode::POINTER);
			}
			EditInputText(ChatSystem::CHAT_MESSAGE_MARKER);

			return true;
		}

		return false;
	}

	EditInputText(keyCode);

	if (keyCode != '\t' && !m_isNavigatingHelpBox)
	{
		m_selectedLineNum = -1;
	}

	return true;
}

bool DevConsole::Event_PasteRequested(EventArgs& args)
{
	UNUSED(args);

	std::scoped_lock<std::recursive_mutex> lock(g_devConsole->m_devConsoleMutex);

	if (m_mode == HIDDEN)
	{
		return false;
	}

	std::string clipboardText = Window::GetClipboardText();
	if (clipboardText.empty())
	{
		return true;
	}

	SanitizePastedText(clipboardText);
	AppendToInputText(clipboardText);

	m_selectedLineNum = -1;
	return true;
}

bool DevConsole::Event_CommandClear(EventArgs& args)
{
	if (GetMode() == HIDDEN)
		return false;

	ClearCommands(args);
	return true;
}

bool DevConsole::Event_CommandHelp(EventArgs& args)
{
	UNUSED(args);
	HelpCommand();
	return true;
}

bool DevConsole::Event_CommandResizeDevConsole(EventArgs& args)
{

	if (GetMode() == HIDDEN)
		return false;

	 ResizeConsoleWindow(args);
	 return true;
}

bool DevConsole::Event_CommandDevConsoleNavigation(EventArgs& args)
{
	UNUSED(args);
	if (GetMode() == HIDDEN)
		return false;

	AddLine(DevConsole::INFO_MAJOR, "-------Dev Console Navigation-------",COMMAND_MESSAGE, 1.f);
	AddLine(DevConsole::INFO_MINOR, "~                  Open/Close Console",COMMAND_MESSAGE, .75f);
	AddLine(DevConsole::INFO_MINOR, "ENTER              Execute Line",COMMAND_MESSAGE, .75f);
	AddLine(DevConsole::INFO_MINOR, "ARROW KEYS (U/D)   Copy Lines",COMMAND_MESSAGE, .75f);
	AddLine(DevConsole::INFO_MINOR, "ARROW KEYS (L/R)   Navigate Cursor",COMMAND_MESSAGE, .75f);
	AddLine(DevConsole::INFO_MINOR, "TAB                Select Suggestion", .75f);
	AddLine(DevConsole::INFO_MINOR, "ESC                Clear Line",COMMAND_MESSAGE, .75f);
	AddLine(DevConsole::INFO_MAJOR, "-----------------------------------",COMMAND_MESSAGE, 1.f);
	return true;
}

bool DevConsole::Event_CommandShowSuggestions(EventArgs& args)
{
	if (GetMode() == HIDDEN)
		return false;

	bool show = args.HasKey("True");
	bool hide = args.HasKey("False");

	if(show && !hide)
		ShowSuggestions(true);
	
	else if (hide && !show)
		ShowSuggestions(false);

	else 
		ShowSuggestions(true);

	return true;
}

bool DevConsole::Event_Echo(EventArgs& args)
{

	std::string clumpedMessage = args.GetValue("message", "");
	Strings messageWords = SplitStringOnDelimiter(clumpedMessage, '+');
	std::string message = messageWords[0];
	for (int i = 1; i < (int)messageWords.size(); ++i)
	{
		message.append(Stringf(" %s", messageWords[i].c_str()));
	}
	AddLine(CHAT_MESSAGE, message);
	return true;
	
}

bool DevConsole::Event_RemoteCommand(EventArgs& args)
{
	UNUSED(args);
	/*This event will never be heard locally, 
	* the RemoteCmd event just parses the arguments into a valid command,
	* And sends it to g_networkSystem.
	* 
	* The cmd= argument is required.
	* e.g. RemoteCmd cmd=Help will call the Help command on the other computer's DevConsole if the client is listening for it
	* 
	* I included a local subscription to it so that DevConsole UI hints to user that it can be called
	* 
	* */
	return false;
}

bool DevConsole::Event_Chat(EventArgs& args)
{
	if(!m_chatSystem)
		return false;


	std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

	if (args.HasKey("Isolate"))
	{
		m_showOnlyChat = args.GetValue("isolate", true);
		if(m_showOnlyChat)
		{
			m_hideChat = false;
		}
	}

	else if (args.HasKey("Hide"))
	{
		m_hideChat = args.GetValue("hide", true);
		if(!m_hideChat)
		{
			//g_devConsole->m_showOnlyChat = false;
		}
		else if (m_showOnlyChat)
		{
			m_showOnlyChat = false;
		}
	}

	std::string currentChatName = m_chatSystem->GetName();
	m_chatSystem->SetName(args.GetValue("name", currentChatName));

	return true;
}

bool DevConsole::Event_Notifications(EventArgs& args)
{

	if (args.HasKey("hide"))
	{
		std::scoped_lock<std::recursive_mutex> lock(g_devConsole->m_devConsoleMutex);

		std::string hideString = args.GetValue("hide", "all");
		hideString = GetLowercase(hideString);
		if (hideString == "all")
		{
			for (int i = 0; i < NUM_DEVCONSOLE_LINE_TYPES; ++i)
			{
				m_visibleAlerts[i] = false;
			}

			return true;
		}

		else
		{
			DevConsoleLineType lineType = DevConsole::GetLineTypeFromNotificationString(hideString);
			if(lineType == INVALID_LINE_TYPE)
				return false;

			m_visibleAlerts[lineType] = false;
			return true;
		}
	}

	if (args.HasKey("show"))
	{
		std::scoped_lock<std::recursive_mutex> lock(m_devConsoleMutex);

		std::string showString = args.GetValue("show", "all");
		showString = GetLowercase(showString);
		if (showString == "all")
		{
			m_visibleAlerts[COMMAND_REMOTE] = true;
			m_visibleAlerts[ALERT_MESSAGE] = true;
			m_visibleAlerts[CHAT_MESSAGE_REMOTE] = true;
			return true;
		}

		else
		{
			DevConsoleLineType lineType = DevConsole::GetLineTypeFromNotificationString(showString);
			if (lineType == INVALID_LINE_TYPE)
				return false;

			m_visibleAlerts[lineType] = true;
			return true;
		}
	}
	return false;
}

bool DevConsole::Event_ExecuteXmlCommandScriptFile(EventArgs& args)
{
	std::string fileName = args.GetValue("Name", "");
	ExecuteXmlCommandScriptFile(fileName);
	return true;
}





