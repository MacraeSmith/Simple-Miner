#pragma once
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/HashedCaseInsensitiveString.hpp"
#include "Engine/Core/NamedProperties.hpp"
#include <string>
#include <vector>
#include <map>
#include <mutex>

//typedef NamedStrings EventArgs; //#TODO: change to NamedProperties in SD4
typedef NamedProperties EventArgs;
typedef bool(EventCallbackFunction)(EventArgs& args);

struct EventSystemConfig
{
};

struct EventSubscriber
{
	virtual ~EventSubscriber();
};

struct EventSubscriptionBase
{
	virtual ~EventSubscriptionBase() {};
	virtual bool Execute(EventArgs& args) = 0;
};


struct EventFunctionSubscription : public EventSubscriptionBase
{
	EventFunctionSubscription(EventCallbackFunction func)
	:m_callbackFunction(func) {};

	virtual bool Execute(EventArgs& args) override
	{
		return m_callbackFunction(args);
	}

	EventCallbackFunction* m_callbackFunction = nullptr;
};


template <typename T>
struct EventObjectMethodSubscription : public EventSubscriptionBase
{
	typedef bool (T::* EventObjectMethodPtr)(EventArgs& args);
	EventObjectMethodSubscription(T* object, EventObjectMethodPtr method)
	:m_object(object), m_method(method) {};

	virtual bool Execute(EventArgs& args) override
	{
		return (m_object->*m_method)(args);
	}

	T* m_object = nullptr;
	EventObjectMethodPtr m_method = nullptr;
};

typedef std::vector<EventSubscriptionBase*> SubscriptionList;

struct EventInfo
{
	HashedCaseInsensitiveString m_name;
	Strings m_argumentFormats;
	bool m_visibleEvent = true; // will be displayed when Help Command is called
};

class EventSystem
{
public:
	explicit EventSystem(EventSystemConfig const& config);
	~EventSystem(){}

	void Startup();
	void ShutDown();
	void BeginFrame();
	void EndFrame();

	void SubscribeEventCallbackFunction(HashedCaseInsensitiveString const& eventName, EventCallbackFunction* function, bool visibleEvent = true);
	void SubscribeEventCallbackFunction(HashedCaseInsensitiveString const& eventName, Strings const& argumentFormats, EventCallbackFunction* function, bool visibleEvent = true);
	void UnsubscribeEventCallbackFunction(HashedCaseInsensitiveString const& eventName, EventCallbackFunction* function);
	void UnsubscribeAllEventCallbacks(HashedCaseInsensitiveString const& eventName);

	template <typename T>
	void SubscribeEventCallbackObjectMethod(HashedCaseInsensitiveString const& eventName, T* objectPtr, bool(T::* methodPtr)(EventArgs& args), bool visibleEvent = true)
	{
		std::scoped_lock<std::recursive_mutex> lock(m_eventSystemMutex);

		SubscriptionList& subscribers = m_subscriptionListByEventName[eventName];
		EventObjectMethodSubscription<T>* newSubscriber = new EventObjectMethodSubscription<T>(objectPtr, methodPtr);
		subscribers.push_back(newSubscriber);

		EventInfo eventInfo;
		eventInfo.m_name = eventName;
		eventInfo.m_visibleEvent = visibleEvent;
		m_eventNameInfoPair[eventName] = eventInfo;
	}

	template <typename T> 
	void SubscribeEventCallbackObjectMethod(HashedCaseInsensitiveString const& eventName, Strings const& argumentFormats, T* objectPtr, bool(T::* methodPtr)(EventArgs& args), bool visibleEvent = true)
	{
		std::scoped_lock<std::recursive_mutex> lock(m_eventSystemMutex);
		SubscriptionList& subscribers = m_subscriptionListByEventName[eventName];
		EventObjectMethodSubscription<T>* newSubscriber = new EventObjectMethodSubscription<T>(objectPtr, methodPtr);
		subscribers.push_back(newSubscriber);

		EventInfo eventInfo;
		eventInfo.m_name = eventName;
		eventInfo.m_visibleEvent = visibleEvent;
		for (int argumentNum = 0; argumentNum < (int)argumentFormats.size(); ++argumentNum)
		{
			eventInfo.m_argumentFormats.push_back(argumentFormats[argumentNum]);
		}
		m_eventNameInfoPair[eventName] = eventInfo;
	}

	template <typename T>
	void UnsubscribeEventCallbackObjectMethod(HashedCaseInsensitiveString const& eventName, T* objectPtr, bool(T::* methodPtr)(EventArgs& args))
	{
		std::scoped_lock<std::recursive_mutex> lock(m_eventSystemMutex);

		auto subscriptionListIter = m_subscriptionListByEventName.find(eventName);
		if (subscriptionListIter == m_subscriptionListByEventName.end())
			return;

		SubscriptionList& subList = subscriptionListIter->second;
		for (int functionNum = 0; functionNum < (int)subList.size(); ++functionNum)
		{
			EventObjectMethodSubscription<T>* subscription = dynamic_cast<EventObjectMethodSubscription<T>*>(subList[functionNum]);
			if(!subscription)
				continue;

			if (subscription->m_object == objectPtr && subscription->m_method == methodPtr)
			{
				subList.erase(subList.begin() + functionNum);
				return;
			}
		}
	}

	template <typename T>
	void UnsubscribeAllEventCallbacksForObject(T* objectPtr)
	{
		std::scoped_lock<std::recursive_mutex> lock(m_eventSystemMutex);

		for (std::pair<HashedCaseInsensitiveString const, SubscriptionList>& element : m_subscriptionListByEventName)
		{
			SubscriptionList& subList = element.second;
			for (int functionNum = 0; functionNum < (int)subList.size(); ++functionNum)
			{
				EventObjectMethodSubscription<T>* subscription = dynamic_cast<EventObjectMethodSubscription<T>*>(subList[functionNum]);
				if(!subscription)
					continue;

				if (subscription->m_object == objectPtr)
				{
					subList.erase(subList.begin() + functionNum);
				}
			}
		}
	}



	bool FireEvent(HashedCaseInsensitiveString const& eventName, EventArgs& args);
	bool FireEvent(HashedCaseInsensitiveString const& eventName);

	bool IsValidEvent(std::string const& eventInfo) const;
	HashedStrings GetAllRegisteredEventNames() const;
	std::vector<EventInfo> GetAllRegisteredEventInfos() const;
	bool GetArgumentFormatsForEventName(HashedCaseInsensitiveString const& eventName, Strings& out_strings) const;

protected:


protected:
	EventSystemConfig m_config;
	std::map<HashedCaseInsensitiveString, SubscriptionList> m_subscriptionListByEventName;
	std::map<HashedCaseInsensitiveString, EventInfo> m_eventNameInfoPair;
	mutable std::recursive_mutex m_eventSystemMutex;

};

void SubscribeEventCallbackFunction(HashedCaseInsensitiveString const& eventName, EventCallbackFunction* function, bool visibleEvent = true);
void SubscribeEventCallbackFunction(HashedCaseInsensitiveString const& eventName, Strings const& argumentFormats, EventCallbackFunction* function, bool visibleEvent = true);
void UnsubscribeEventCallbackFunction(HashedCaseInsensitiveString const& eventName, EventCallbackFunction* function);
void UnsubscribeAllEventCallbacks(HashedCaseInsensitiveString const& eventName);
bool FireEvent(HashedCaseInsensitiveString const& eventName, EventArgs& args);
bool FireEvent(HashedCaseInsensitiveString const& eventName);

extern EventSystem* g_eventSystem;
template<typename T>
void SubscribeEventCallbackObjectMethod(HashedCaseInsensitiveString const& eventName, T* objectPtr, bool(T::* methodPtr)(EventArgs& args), bool visibleEvent = true)
{
	if (g_eventSystem != nullptr)
	{
		g_eventSystem->SubscribeEventCallbackObjectMethod(eventName, objectPtr, methodPtr, visibleEvent);
	}
}

template<typename T>
void SubscribeEventCallbackObjectMethod(HashedCaseInsensitiveString const& eventName, Strings arguments, T* objectPtr, bool(T::* methodPtr)(EventArgs& args), bool visibleEvent = true)
{
	if (g_eventSystem != nullptr)
	{
		g_eventSystem->SubscribeEventCallbackObjectMethod(eventName, arguments, objectPtr, methodPtr, visibleEvent);
	}
}

template<typename T>
void UnsubscribeEventCallbackObjectMethod(HashedCaseInsensitiveString const& eventName, T* objectPtr, bool(T::* methodPtr)(EventArgs& args))
{
	if (g_eventSystem != nullptr)
	{
		g_eventSystem->UnsubscribeEventCallbackObjectMethod(eventName, objectPtr, methodPtr);
	}
}

template<typename T>
void UnsubscribeAllEventCallbacksForObject(T* objectPtr)
{
	if (g_eventSystem != nullptr)
	{
		g_eventSystem->UnsubscribeAllEventCallbacksForObject(objectPtr);
	}
}