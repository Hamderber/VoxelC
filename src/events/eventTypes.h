#pragma once

#include <stdbool.h>
#include "events/context/CtxInputRaw_t.h"
#include "events/context/CtxInputMapped_t.h"

#define MAX_EVENT_LISTENERS 128

typedef enum
{
    EVENT_TYPE_NONE,
    EVENT_TYPE_INPUT_RAW,
    EVENT_TYPE_INPUT_MAPPED,
    EVENT_TYPE_PHYSICS_COLLISION,
} EventType_t;

typedef union
{
    CtxInputRaw_t *inputRaw;
    CtxInputMapped_t *inputMapped;
    void *generic;
} EventData_t;

typedef struct
{
    EventType_t type;
    EventData_t data;
} Event_t;

typedef enum
{
    // Normal
    EVENT_RESULT_PASS,
    // Caught an error
    EVENT_RESULT_ERROR
} EventResult_t;

struct State_t;
typedef EventResult_t (*EventCallbackFn)(struct State_t *state, Event_t *event, void *context);

typedef struct
{
    EventCallbackFn fn;
    // Unsubscribe once invoked
    bool consumeListener;
    bool consumeEvent;
    void *subscribeContext;
} EventListener_t;

// Event system for a specific channel
typedef struct
{
    EventListener_t eventListeners[MAX_EVENT_LISTENERS];
} EventSystem_t;

typedef enum
{
    EVENT_SUBSCRIBE_RESULT_FAIL = -1,
    EVENT_SUBSCRIBE_RESULT_PASS = 1,
} EventSubscribeResult_t;

typedef enum
{
    // window resize, shutdown, etc.
    EVENT_CHANNEL_APP,
    // mouse/keyboard
    EVENT_CHANNEL_INPUT,
    // rendering updates
    EVENT_CHANNEL_GRAPHICS,
    // collision events, triggers
    EVENT_CHANNEL_PHYSICS,
    // spawn, death, transform changes
    EVENT_CHANNEL_ENTITY,
    // Keep this last so it represents the enum quantity
    EVENT_CHANNEL_COUNT,
} EventChannelID_t;

static const char *EVENT_CHANNEL_NAMES[] = {
    "EVENT_CHANNEL_APP",
    "EVENT_CHANNEL_INPUT",
    "EVENT_CHANNEL_GRAPHICS",
    "EVENT_CHANNEL_PHYSICS",
    "EVENT_CHANNEL_ENTITY",
};

typedef struct
{
    EventChannelID_t ID;
    EventSystem_t eventSystem;
} EventChannel_t;

typedef struct
{
    // There is a channel for every type of EventChannelID_t
    EventChannel_t channels[EVENT_CHANNEL_COUNT];
} EventBus_t;
