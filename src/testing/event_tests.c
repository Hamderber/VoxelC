#include "core/logs.h"
#include "core/types/state_t.h"
#include "core/logs.h"
#include "events/eventTypes.h"
#include "events/eventBus.h"
#include "core/types/state_t.h"
#include <string.h>

// Dummy test data for event payload
typedef struct
{
    int testValue;
} EventTestPayload_t;

static EventResult_t onGenericPass(struct State_t *state, Event_t *event, void *ctx)
{
    EventTestPayload_t *payload = (EventTestPayload_t *)event->data;
    logs_log(LOG_UNIT_TEST, "onGenericPass received value = %d", payload->testValue);
    payload->testValue += 1;
    return EVENT_RESULT_PASS;
}

static EventResult_t onConsumeListener(struct State_t *state, Event_t *event, void *ctx)
{
    EventTestPayload_t *payload = (EventTestPayload_t *)event->data;
    logs_log(LOG_UNIT_TEST, "onConsumeListener fired value = %d", payload->testValue);
    payload->testValue += 10;
    return EVENT_RESULT_PASS;
}

static EventResult_t onConsumeEvent(struct State_t *state, Event_t *event, void *ctx)
{
    EventTestPayload_t *payload = (EventTestPayload_t *)event->data;
    logs_log(LOG_UNIT_TEST, "onConsumeEvent fired value = %d will stop propagation", payload->testValue);
    payload->testValue += 100;
    return EVENT_RESULT_PASS;
}

static EventResult_t onIntentionalError(struct State_t *state, Event_t *event, void *ctx)
{
    return EVENT_RESULT_ERROR;
}

/// @brief Verify subscribe/unsubscribe/consume/error behavior
/// @return number of failed checks
static int eventTests_subscribe(State_t *state, EventBus_t *bus)
{
    int failures = 0;
    Event_t evt;
    EventTestPayload_t payload;

    logs_log(LOG_UNIT_TEST, "Subscribe basic event listeners...");
    events_subscribe(bus, EVENT_CHANNEL_INPUT, onGenericPass, false, false, NULL);
    events_subscribe(bus, EVENT_CHANNEL_INPUT, onConsumeListener, true, false, NULL);
    events_subscribe(bus, EVENT_CHANNEL_INPUT, onConsumeEvent, false, true, NULL);
    events_subscribe(bus, EVENT_CHANNEL_INPUT, onIntentionalError, false, false, NULL);

    logs_log(LOG_UNIT_TEST, "Expecting to catch a NULL callback function during subscription... (Error expected)");
    EventSubscribeResult_t badSub = events_subscribe(bus, EVENT_CHANNEL_INPUT, NULL, false, false, NULL);
    if (badSub != EVENT_SUBSCRIBE_RESULT_FAIL)
    {
        logs_log(LOG_UNIT_TEST, "Null-function subscription should've failed! (But it didn't)");
        failures++;
    }

    logs_log(LOG_UNIT_TEST, "Expecting to catch a NULL event bus during callback subscription... (Error expected)");
    EventSubscribeResult_t badBus = events_subscribe(NULL, EVENT_CHANNEL_INPUT, onGenericPass, false, false, NULL);
    if (badBus != EVENT_SUBSCRIBE_RESULT_FAIL)
    {
        logs_log(LOG_UNIT_TEST, "Null-bus subscription should've failed! (But it didn't)");
        failures++;
    }

    payload.testValue = 1;
    evt = (Event_t){.type = EVENT_TYPE_INPUT, .data = &payload};

    logs_log(LOG_UNIT_TEST, "Publish event to INPUT channel...");
    events_publish(state, bus, EVENT_CHANNEL_INPUT, evt);

    // after publish:
    // onGenericPass +1, onConsumeListener +10 (then unsubscribed), onConsumeEvent +100 (then stops others)
    // final expected = 1+1+10+100 = 112
    if (payload.testValue != 112)
    {
        logs_log(LOG_UNIT_TEST, "Event propagation produced incorrect value %d != 112", payload.testValue);
        failures++;
    }

    // The consumeListener function should have been unsubscribed automatically.
    payload.testValue = 5;
    evt.data = &payload;
    logs_log(LOG_UNIT_TEST, "Publish again after consumeListener unsubscribe...");
    events_publish(state, bus, EVENT_CHANNEL_INPUT, evt);

    // Should no longer add +10 (only +1 from generic, +100 from consumeEvent)
    if (payload.testValue != 106)
    {
        logs_log(LOG_UNIT_TEST, "ConsumeListener was not removed correctly %d != 106", payload.testValue);
        failures++;
    }

    logs_log(LOG_UNIT_TEST, "Manual unsubscribe...");
    events_unsubscribe(bus, EVENT_CHANNEL_INPUT, onGenericPass);
    payload.testValue = 0;
    events_publish(state, bus, EVENT_CHANNEL_INPUT, evt);
    // only onConsumeEvent should fire (+100)
    if (payload.testValue != 100)
    {
        logs_log(LOG_UNIT_TEST, "Unsubscribe failed or listener still active %d != 100", payload.testValue);
        failures++;
    }

    // Reset event channel for isolated error test
    memset(&bus->channels[EVENT_CHANNEL_INPUT].eventSystem, 0, sizeof(EventSystem_t));
    logs_log(LOG_UNIT_TEST, "Intentional error callback handling... (Error expected)");
    events_subscribe(bus, EVENT_CHANNEL_INPUT, onIntentionalError, false, false, NULL);
    payload.testValue = 999;
    events_publish(state, bus, EVENT_CHANNEL_INPUT, evt);
    // onIntentionalError just returns ERROR no testValue change but should not crash
    if (payload.testValue != 999)
    {
        logs_log(LOG_UNIT_TEST, "Error handler modified payload unexpectedly");
        failures++;
    }

    // Clear channel again before duplicate test
    memset(&bus->channels[EVENT_CHANNEL_INPUT].eventSystem, 0, sizeof(EventSystem_t));
    logs_log(LOG_UNIT_TEST, "Testing duplicate subscription handling... (Error expected)");
    events_subscribe(bus, EVENT_CHANNEL_INPUT, onGenericPass, false, false, NULL);
    events_subscribe(bus, EVENT_CHANNEL_INPUT, onGenericPass, false, false, NULL);
    payload.testValue = 1;
    events_publish(state, bus, EVENT_CHANNEL_INPUT, evt);
    if (payload.testValue != 2)
    {
        logs_log(LOG_UNIT_TEST, "Error when testing duplicate listener subscriptions.");
        failures++;
    }

    // cleanup
    for (int i = 0; i < EVENT_CHANNEL_COUNT; i++)
        memset(&bus->channels[i].eventSystem, 0, sizeof(EventSystem_t));

    return failures;
}

static int eventTests_tests(State_t *state)
{
    logs_log(LOG_UNIT_TEST, "Running event system tests...");

    int failures = 0;
    EventBus_t *bus = &state->eventBus;

    failures += eventTests_subscribe(state, bus);

    return failures;
}

void eventTests_run(State_t *state)
{
    // The event bus is already initialized here
    logs_logIfError(eventTests_tests(state), "Event test(s) failed!");

    // Re-initalize event buss after testing (clears dangling listeners)
    events_init(&state->eventBus);
}
