#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/StringUtils.hpp"


EventSystem::EventSystem(EventSystemConfig const& config)
	:m_config(config)
{
}

void EventSystem::Startup()
{
}

void EventSystem::ShutDown()
{
}

void EventSystem::BeginFrame()
{
}

void EventSystem::EndFrame()
{
}

void EventSystem::SubscribeEventCallbackFunction(HashedCaseInsensitiveString const& eventName, EventCallbackFunction* function, bool visibleEvent)
{
	std::scoped_lock<std::recursive_mutex> lock(m_eventSystemMutex);

	EventFunctionSubscription* newSubscription = new EventFunctionSubscription(function);
	m_subscriptionListByEventName[eventName].push_back(newSubscription);

	EventInfo eventInfo;
	eventInfo.m_name = eventName;
	eventInfo.m_visibleEvent = visibleEvent;
	m_eventNameInfoPair[eventName] = eventInfo;
}

void EventSystem::SubscribeEventCallbackFunction(HashedCaseInsensitiveString const& eventName, Strings const& argumentFormats, EventCallbackFunction* function, bool visibleEvent)
{
	std::scoped_lock<std::recursive_mutex> lock(m_eventSystemMutex);

	EventFunctionSubscription* newSubscription = new EventFunctionSubscription(function);

	m_subscriptionListByEventName[eventName].push_back(newSubscription);

	EventInfo eventInfo;
	eventInfo.m_name = eventName;
	eventInfo.m_visibleEvent = visibleEvent;
	for (int argumentNum = 0; argumentNum < (int)argumentFormats.size(); ++argumentNum)
	{
		eventInfo.m_argumentFormats.push_back(argumentFormats[argumentNum]);
	}
	m_eventNameInfoPair[eventName] = eventInfo;
}

void EventSystem::UnsubscribeEventCallbackFunction(HashedCaseInsensitiveString const& eventName, EventCallbackFunction* function)
{
	std::scoped_lock<std::recursive_mutex> lock(m_eventSystemMutex);

	auto subscriptionListIter = m_subscriptionListByEventName.find(eventName);
	if (subscriptionListIter == m_subscriptionListByEventName.end())
		return;
	
	SubscriptionList& subList = subscriptionListIter->second;
	for (int functionNum = 0; functionNum < 
		(subList.size()); ++functionNum)
	{
		EventFunctionSubscription* subscription = dynamic_cast<EventFunctionSubscription*>(subList[functionNum]);
		if (subscription && subscription->m_callbackFunction == function)
		{
			subList.erase(subList.begin() + functionNum);
			return;
		}
	}
}

void EventSystem::UnsubscribeAllEventCallbacks(HashedCaseInsensitiveString const& eventName)
{
	std::scoped_lock<std::recursive_mutex> lock(m_eventSystemMutex);

	auto foundEvent = m_subscriptionListByEventName.find(eventName);
	if (foundEvent != m_subscriptionListByEventName.end())
	{
		SubscriptionList& subList = foundEvent->second;
		subList.clear();
	}
}

bool EventSystem::FireEvent(HashedCaseInsensitiveString const& eventName, EventArgs& args)
{
	std::scoped_lock<std::recursive_mutex> lock(m_eventSystemMutex);

	auto subscriptionListIter = m_subscriptionListByEventName.find(eventName);
	if (subscriptionListIter == m_subscriptionListByEventName.end())
		return false;

	SubscriptionList& subList = subscriptionListIter->second;
	for (int functionNum = 0; functionNum < (int)(subList.size()); ++functionNum)
	{
		bool callbackReturn = subList[functionNum]->Execute(args);
		if (callbackReturn)
			return true;
	}

	return false;
}

bool EventSystem::FireEvent(HashedCaseInsensitiveString const& eventName)
{
	std::scoped_lock<std::recursive_mutex> lock(m_eventSystemMutex);

	auto subscriptionListIter = m_subscriptionListByEventName.find(eventName);
	if (subscriptionListIter == m_subscriptionListByEventName.end())
		return false;
		

	SubscriptionList& subList = subscriptionListIter->second;
	EventArgs args;
	for (int functionNum = 0; functionNum < (int)(subList.size()); ++functionNum)
	{
		bool callbackReturn = subList[functionNum]->Execute(args);
		if (callbackReturn)
			return true;
	}

	return false;
}

bool EventSystem::IsValidEvent(std::string const& eventInfo) const
{
	std::scoped_lock<std::recursive_mutex> lock(m_eventSystemMutex);

	Strings eventInfoSplit = SplitStringOnDelimiter(eventInfo, ' ', false);
	if (eventInfoSplit.size() <= 0)
		return false;

	HashedCaseInsensitiveString hashedEventName(eventInfoSplit[0]);
	
	auto subscriptionListIter = m_subscriptionListByEventName.find(hashedEventName);
	return (subscriptionListIter != m_subscriptionListByEventName.end());
}

HashedStrings EventSystem::GetAllRegisteredEventNames() const
{
	std::scoped_lock<std::recursive_mutex> lock(m_eventSystemMutex);

	HashedStrings eventNames;

	for (const auto& [key, value] : m_subscriptionListByEventName)
	{
		auto found = m_eventNameInfoPair.find(key);
		if (found != m_eventNameInfoPair.end())
		{
			
			eventNames.push_back(found->second.m_name);
			
			continue;
		}

		eventNames.push_back(key);
	}

	return eventNames;
}

std::vector<EventInfo> EventSystem::GetAllRegisteredEventInfos() const
{
	std::scoped_lock<std::recursive_mutex> lock(m_eventSystemMutex);

	std::vector<EventInfo> eventInfos;
	for (const auto& [key, value] : m_eventNameInfoPair)
	{
		auto found = m_eventNameInfoPair.find(key);
		if (found != m_eventNameInfoPair.end())
		{

			eventInfos.push_back(found->second);

			continue;
		}
	}

	return eventInfos;
}

bool EventSystem::GetArgumentFormatsForEventName(HashedCaseInsensitiveString const& eventName, Strings& out_strings) const
{
	std::scoped_lock<std::recursive_mutex> lock(m_eventSystemMutex);

	auto found = m_eventNameInfoPair.find(eventName);
	if (found != m_eventNameInfoPair.end())
	{
		Strings argumentFormats = found->second.m_argumentFormats;
		if (argumentFormats.size() > 0)
		{
			for (int argumentNum = 0; argumentNum < (int)argumentFormats.size(); ++argumentNum)
			{
				argumentFormats[argumentNum].insert(0, " ");
				argumentFormats[argumentNum].append(" ");
			}

			out_strings = argumentFormats;
			return true;
		}
		return false;
	}
	return false;
}




//Standalone Functions
//----------------------------------------------------------------------------------------------------------------
void SubscribeEventCallbackFunction(HashedCaseInsensitiveString const& eventName, EventCallbackFunction* function, bool visibleEvent)
{
	if (g_eventSystem != nullptr)
	{
		g_eventSystem->SubscribeEventCallbackFunction(eventName, function, visibleEvent);
	}
}

void SubscribeEventCallbackFunction(HashedCaseInsensitiveString const& eventName, Strings const& argumentFormats, EventCallbackFunction* function, bool visibleEvent)
{
	if (g_eventSystem != nullptr)
	{
		g_eventSystem->SubscribeEventCallbackFunction(eventName, argumentFormats, function, visibleEvent);
	}
}

void UnsubscribeEventCallbackFunction(HashedCaseInsensitiveString const& eventName, EventCallbackFunction* function)
{
	if (g_eventSystem != nullptr)
	{
		g_eventSystem->UnsubscribeEventCallbackFunction(eventName, function);
	}
}

void UnsubscribeAllEventCallbacks(HashedCaseInsensitiveString const& eventName)
{
	if (g_eventSystem)
	{
		g_eventSystem->UnsubscribeAllEventCallbacks(eventName);
	}
}

bool FireEvent(HashedCaseInsensitiveString const& eventName, EventArgs& args)
{
	if (g_eventSystem != nullptr)
	{
		return g_eventSystem->FireEvent(eventName, args);
	}

	return false;
}

bool FireEvent(HashedCaseInsensitiveString const& eventName)
{
	if (g_eventSystem != nullptr)
	{
		return g_eventSystem->FireEvent(eventName);
	}
	
	return false;
}


EventSubscriber::~EventSubscriber()
{
	if (g_eventSystem)
	{
		g_eventSystem->UnsubscribeAllEventCallbacksForObject(this);
	}
}
