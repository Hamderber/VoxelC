#pragma once

#include <stdbool.h>
#include "events/context/CtxInputRaw_t.h"
#include "events/context/CtxInputMapped_t.h"
#include "events/context/CtxGUI_t.h"
#include "events/context/CtxChunk_t.h"

#define MAX_EVENT_LISTENERS 128

typedef enum EventType_e
{
    EVENT_TYPE_NONE,
    EVENT_TYPE_INPUT_RAW,
    EVENT_TYPE_INPUT_MAPPED,
    EVENT_TYPE_PHYSICS_COLLISION,
    EVENT_TYPE_GUI,
    EVENT_TYPE_CHUNK_LOAD,
    EVENT_TYPE_CHUNK_UNLOAD,
    EVENT_TYPE_CHUNK_PLAYER_CHUNKPOS_CHANGE,
    EVENT_TYPE_PLAYER_JOIN,
    EVENT_TYPE_PLAYER_LEAVE,
} EventType_e;

// Must be pointers
typedef union
{
    CtxInputRaw_t *pInputRaw;
    CtxInputMapped_t *pInputMapped;
    CtxGUI_t *pGui;
    CtxChunk_t *pChunkEvntData;
    void *pGeneric;
} EventData_t;

typedef struct Event_t
{
    EventType_e type;
    EventData_t data;
} Event_t;

typedef enum EventResult_e
{
    // Normal
    EVENT_RESULT_PASS,
    // Consume the event
    EVENT_RESULT_CONSUME,
    // Caught an error
    EVENT_RESULT_ERROR
} EventResult_e;

struct State_t;
typedef EventResult_e (*EventCallbackFn)(struct State_t *pState, Event_t *pEvent, void *pCtx);

typedef struct
{
    EventCallbackFn fn;
    // Unsubscribe once invoked
    bool consumeListener;
    // Use this to decide if the event should return EVENT_RESULT_CONSUME
    bool consumeEvent;
    void *pSubscribeContext;
} EventListener_t;

// Event system for a specific channel
typedef struct
{
    EventListener_t eventListeners[MAX_EVENT_LISTENERS];
} EventSystem_t;

typedef enum EventSubscribeResult_e
{
    EVENT_SUBSCRIBE_RESULT_FAIL = -1,
    EVENT_SUBSCRIBE_RESULT_PASS = 1,
} EventSubscribeResult_e;

typedef enum EventChannelID_e
{
    // window resize, shutdown, etc.
    EVENT_CHANNEL_APP,
    // input actions
    EVENT_CHANNEL_INPUT_ACTIONS,
    // keycode/mouse
    EVENT_CHANNEL_INPUT_RAW,
    // rendering updates
    EVENT_CHANNEL_GRAPHICS,
    // collision events, triggers
    EVENT_CHANNEL_PHYSICS,
    // spawn, death, transform changes
    EVENT_CHANNEL_ENTITY,
    // menu events
    EVENT_CHANNEL_GUI,
    // chunk-based events
    EVENT_CHANNEL_CHUNK,
    // player related events like join, leave, death, etc
    EVENT_CHANNEL_PLAYER,
    // Keep this last so it represents the enum quantity
    EVENT_CHANNEL_COUNT,
} EventChannelID_e;

static const char *pEVENT_CHANNEL_NAMES[] = {
    "EVENT_CHANNEL_APP",
    "EVENT_CHANNEL_INPUT_ACTIONS",
    "EVENT_CHANNEL_INPUT_RAW",
    "EVENT_CHANNEL_GRAPHICS",
    "EVENT_CHANNEL_PHYSICS",
    "EVENT_CHANNEL_ENTITY",
    "EVENT_CHANNEL_GUI",
    "EVENT_CHANNEL_CHUNK",
    "EVENT_CHANNEL_PLAYER",
};

typedef struct
{
    EventChannelID_e ID;
    EventSystem_t eventSystem;
} EventChannel_t;

typedef struct EventBus_t
{
    // There is a channel for every type of EventChannelID_e
    EventChannel_t channels[EVENT_CHANNEL_COUNT];
} EventBus_t;
