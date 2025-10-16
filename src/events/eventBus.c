#include "events/eventTypes.h"
#include "core/logs.h"
#include <string.h>
#include "core/types/state_t.h"

// Allows duplicate listeners (undesired but maybe useful?)
EventSubscribeResult_t events_listenerIndexFirst(EventSystem_t *sys, int *index)
{
    for (size_t i = 0; i < MAX_EVENT_LISTENERS; i++)
    {
        if (sys->eventListeners[i].fn == NULL)
        {
            *index = (int)i;
            return EVENT_SUBSCRIBE_RESULT_PASS;
        }
    }

    return EVENT_SUBSCRIBE_RESULT_FAIL;
}

// Prevents duplicates (default desired behaviour)
EventSubscribeResult_t events_listenerIndexSingleton(EventSystem_t *sys, EventCallbackFn fn, int *index)
{
    bool indexFound = false;

    for (size_t i = 0; i < MAX_EVENT_LISTENERS; i++)
    {
        // Force singleton behaviour for EventCallbackFn
        if (sys->eventListeners[i].fn == fn)
        {
            logs_log(LOG_ERROR, "Attempted to subscribe to event system %p with duplicate event callback function %p!",
                     (void *)sys, (void *)fn);

            return EVENT_SUBSCRIBE_RESULT_FAIL;
        }

        // Assign the value of the first available index found
        if (!indexFound && sys->eventListeners[i].fn == NULL)
        {
            *index = (int)i;
            indexFound = true;
        }
    }

    if (indexFound)
    {
        return EVENT_SUBSCRIBE_RESULT_PASS;
    }
    else
    {
        logs_log(LOG_ERROR, "Failed to assign an index for EventCallbackFn %p. There are MAX_EVENT_LISTENERS [%d] assigned!",
                 (void *)fn, MAX_EVENT_LISTENERS);
        return EVENT_SUBSCRIBE_RESULT_FAIL;
    }
}

EventSubscribeResult_t events_subscribe(EventBus_t *bus, EventChannelID_t id, EventCallbackFn fn, bool consumeListener,
                                        bool consumeEvent, void *subCtx)
{
    if (!bus)
    {
        // Don't dereference when logging the error
        logs_log(LOG_ERROR, "Attempted to subscribe to invalid event bus %p with EventCallbackFn %p!", (void *)bus, (void *)fn);
        return EVENT_SUBSCRIBE_RESULT_FAIL;
    }

    if (!fn)
    {
        logs_log(LOG_ERROR, "Attempted to subscribe with invalid EventCallbackFn %p to bus %p!", (void *)fn, (void *)bus);
        return EVENT_SUBSCRIBE_RESULT_FAIL;
    }

    EventSystem_t *sys = &bus->channels[id].eventSystem;
    int index;
    if (events_listenerIndexSingleton(sys, fn, &index) == EVENT_SUBSCRIBE_RESULT_PASS)
    {
        sys->eventListeners[index].fn = fn;
        sys->eventListeners[index].consumeListener = consumeListener;
        sys->eventListeners[index].consumeEvent = consumeEvent;
        sys->eventListeners[index].subscribeContext = subCtx;

        return EVENT_SUBSCRIBE_RESULT_PASS;
    }
    else
    {
        logs_log(LOG_ERROR, "Failed to subscribe event listener to %s", EVENT_CHANNEL_NAMES[(int)id]);
        return EVENT_SUBSCRIBE_RESULT_FAIL;
    }
}

EventSubscribeResult_t events_unsubscribe(EventBus_t *bus, EventChannelID_t id, EventCallbackFn fn)
{
    EventSystem_t *sys = &bus->channels[id].eventSystem;
    for (size_t i = 0; i < MAX_EVENT_LISTENERS; i++)
    {
        if (sys->eventListeners[i].fn == fn)
        {
            sys->eventListeners[i].fn = NULL;
            sys->eventListeners[i].consumeListener = false;
            sys->eventListeners[i].consumeEvent = false;
            sys->eventListeners[i].subscribeContext = NULL;

            return EVENT_SUBSCRIBE_RESULT_PASS;
        }
    }

    logs_log(LOG_ERROR, "Attempted to unsubscribe invalid listener at address %p", fn);
    return EVENT_SUBSCRIBE_RESULT_FAIL;
}

EventSubscribeResult_t events_unsubscribeCollection(EventBus_t *bus, EventChannelID_t id, EventCallbackFn *fns, size_t count)
{
    EventSubscribeResult_t result = EVENT_SUBSCRIBE_RESULT_PASS;

    for (size_t i = 0; i < count; i++)
    {
        if (events_unsubscribe(bus, id, fns[i]) == EVENT_SUBSCRIBE_RESULT_FAIL)
        {
            result = EVENT_SUBSCRIBE_RESULT_FAIL;
        }
    }

    return result;
}

void events_publish(State_t *state, EventBus_t *bus, EventChannelID_t id, Event_t event)
{
    EventSystem_t *sys = &bus->channels[(int)id].eventSystem;

    size_t numUnsubEvents = 0;
    EventCallbackFn unsubFns[MAX_EVENT_LISTENERS];

    EventListener_t *listener;
    for (size_t i = 0; i < MAX_EVENT_LISTENERS; i++)
    {
        listener = &sys->eventListeners[i];

        if (listener->fn == NULL)
            continue;

        EventResult_t result = listener->fn(state, &event, listener->subscribeContext);
        if (result == EVENT_RESULT_ERROR)
        {
            logs_log(LOG_ERROR, "Error during event listener %d in channel %s! The listener and event will be consumed.",
                     (int)i, EVENT_CHANNEL_NAMES[(int)id]);
            listener->consumeListener = true;
            listener->consumeEvent = true;
        }

        if (listener->consumeListener)
        {
            unsubFns[numUnsubEvents] = listener->fn;
            numUnsubEvents++;
        }

        if (listener->consumeEvent)
            break;
    }

    if (numUnsubEvents > 0 && events_unsubscribeCollection(bus, id, unsubFns, numUnsubEvents) != EVENT_SUBSCRIBE_RESULT_PASS)
    {
        logs_log(LOG_ERROR, "Failed to unsubscribe event collection [%d] from %s!", numUnsubEvents, EVENT_CHANNEL_NAMES[(int)id]);
    }
}

void events_init(EventBus_t *bus)
{
    for (size_t i = 0; i < EVENT_CHANNEL_COUNT; i++)
    {
        bus->channels[i] = (EventChannel_t){
            .ID = (EventChannelID_t)i,
        };

        memset(&bus->channels[i].eventSystem, 0, sizeof(EventSystem_t));

        logs_log(LOG_DEBUG, "Initalized event channel %s", EVENT_CHANNEL_NAMES[(int)i]);
    }
}