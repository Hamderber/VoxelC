#pragma once

#include "events/eventTypes.h"

EventSubscribeResult_t events_listenerIndexFirst(EventSystem_t *sys, int *index);

EventSubscribeResult_t events_subscribe(EventBus_t *bus, EventChannelID_t id, EventCallbackFn fn, bool consumeListener,
                                        bool consumeEvent, void *subCtx);

EventSubscribeResult_t events_unsubscribe(EventBus_t *bus, EventChannelID_t id, EventCallbackFn fn);

EventSubscribeResult_t events_unsubscribeCollection(EventBus_t *bus, EventChannelID_t id, EventCallbackFn *fns, size_t count);

void events_publish(struct State_t *state, EventBus_t *bus, EventChannelID_t id, Event_t event);

void events_init(EventBus_t *bus);