#pragma once

#include "events/eventTypes.h"

EventSubscribeResult_e events_listenerIndexFirst(EventSystem_t *sys, int *index);

EventSubscribeResult_e events_subscribe(EventBus_t *bus, EventChannelID_e id, EventCallbackFn fn, bool consumeListener,
                                        bool consumeEvent, void *subCtx);

EventSubscribeResult_e events_unsubscribe(EventBus_t *bus, EventChannelID_e id, EventCallbackFn fn);

EventSubscribeResult_e events_unsubscribeCollection(EventBus_t *bus, EventChannelID_e id, EventCallbackFn *fns, size_t count);

void events_publish(struct State_t *state, EventBus_t *bus, EventChannelID_e id, Event_t event);

void events_init(EventBus_t *bus);