#include <string.h>
#include "events/eventBus.h"
#include "events/eventTypes.h"
#include "../../src/core/types/state_t.h"
#include "../../tests/unit_tests.h"

static int fails = 0;

// Dummy test data for event payload
typedef struct
{
    int testValue;
} EventTestPayload_t;

static EventResult_t onGenericPass(struct State_t *state, Event_t *event, void *ctx)
{
    // Unused
    state;
    ctx;

    EventTestPayload_t *payload = (EventTestPayload_t *)event->data.pGeneric;
    payload->testValue += 1;
    return EVENT_RESULT_PASS;
}

static EventResult_t onConsumeListener(struct State_t *state, Event_t *event, void *ctx)
{
    // Unused
    state;
    ctx;

    EventTestPayload_t *payload = (EventTestPayload_t *)event->data.pGeneric;
    payload->testValue += 10;
    return EVENT_RESULT_PASS;
}

static EventResult_t onConsumeEvent(struct State_t *state, Event_t *event, void *ctx)
{
    // Unused
    state;
    ctx;

    EventTestPayload_t *payload = (EventTestPayload_t *)event->data.pGeneric;
    payload->testValue += 100;
    return EVENT_RESULT_PASS;
}

static EventResult_t onIntentionalError(struct State_t *state, Event_t *event, void *ctx)
{
    // Unused
    state;
    event;
    ctx;

    return EVENT_RESULT_ERROR;
}

/// @brief Verify subscribe/unsubscribe/consume/error behavior
static void eventTests_subscribe(State_t *state, EventBus_t *bus)
{
    Event_t evt;
    EventTestPayload_t payload;

    events_subscribe(bus, EVENT_CHANNEL_INPUT_ACTIONS, onGenericPass, false, false, NULL);
    events_subscribe(bus, EVENT_CHANNEL_INPUT_ACTIONS, onConsumeListener, true, false, NULL);
    events_subscribe(bus, EVENT_CHANNEL_INPUT_ACTIONS, onConsumeEvent, false, true, NULL);
    events_subscribe(bus, EVENT_CHANNEL_INPUT_ACTIONS, onIntentionalError, false, false, NULL);

    EventSubscribeResult_t badSub = events_subscribe(bus, EVENT_CHANNEL_INPUT_ACTIONS, NULL, false, false, NULL);
    fails += ut_assert(badSub == EVENT_SUBSCRIBE_RESULT_FAIL, "Bad subscriber");

    EventSubscribeResult_t badBus = events_subscribe(NULL, EVENT_CHANNEL_INPUT_ACTIONS, onGenericPass, false, false, NULL);
    fails += ut_assert(badBus == EVENT_SUBSCRIBE_RESULT_FAIL, "Bad event bus");

    payload.testValue = 1;
    evt = (Event_t){.type = EVENT_TYPE_INPUT_RAW, .data.pGeneric = &payload};

    events_publish(state, bus, EVENT_CHANNEL_INPUT_ACTIONS, evt);

    // after publish:
    // onGenericPass +1, onConsumeListener +10 (then unsubscribed), onConsumeEvent +100 (then stops others)
    // final expected = 1+1+10+100 = 112
    fails += ut_assert(payload.testValue == 112, "Event: onGenericPass +1, onConsumeListener +10 (then unsubscribed), onConsumeEvent +100 (then stops others)");

    // The consumeListener function should have been unsubscribed automatically.
    payload.testValue = 5;
    evt.data.pGeneric = &payload;
    events_publish(state, bus, EVENT_CHANNEL_INPUT_ACTIONS, evt);

    // Should no longer add +10 (only +1 from pGeneric, +100 from consumeEvent)
    fails += ut_assert(payload.testValue == 106, "Event: only +1 from pGeneric, +100 from consumeEvent (consume)");

    events_unsubscribe(bus, EVENT_CHANNEL_INPUT_ACTIONS, onGenericPass);
    payload.testValue = 0;
    events_publish(state, bus, EVENT_CHANNEL_INPUT_ACTIONS, evt);
    // only onConsumeEvent should fire (+100)
    fails += ut_assert(payload.testValue == 100, "Event: consume only");

    // Reset event channel for isolated error test
    memset(&bus->channels[EVENT_CHANNEL_INPUT_ACTIONS].eventSystem, 0, sizeof(EventSystem_t));
    events_subscribe(bus, EVENT_CHANNEL_INPUT_ACTIONS, onIntentionalError, false, false, NULL);
    payload.testValue = 999;
    events_publish(state, bus, EVENT_CHANNEL_INPUT_ACTIONS, evt);
    // onIntentionalError just returns ERROR no testValue change but should not crash
    fails += ut_assert(payload.testValue == 999, "Event: Catch subscriber error");

    // Clear channel again before duplicate test
    memset(&bus->channels[EVENT_CHANNEL_INPUT_ACTIONS].eventSystem, 0, sizeof(EventSystem_t));
    events_subscribe(bus, EVENT_CHANNEL_INPUT_ACTIONS, onGenericPass, false, false, NULL);
    events_subscribe(bus, EVENT_CHANNEL_INPUT_ACTIONS, onGenericPass, false, false, NULL);
    payload.testValue = 1;
    events_publish(state, bus, EVENT_CHANNEL_INPUT_ACTIONS, evt);
    fails += ut_assert(payload.testValue == 2, "Duplicate subscriber");

    // cleanup
    for (int i = 0; i < EVENT_CHANNEL_COUNT; i++)
        memset(&bus->channels[i].eventSystem, 0, sizeof(EventSystem_t));
}

int event_tests_run(void)
{
    State_t state = {0};
    events_init(&state.eventBus);
    eventTests_subscribe(&state, &state.eventBus);
    return fails;
}