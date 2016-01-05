#include "pebble.h"

#include "Adventure.h"
#include "BaseWindow.h"
#include "Character.h"
#include "Clock.h"
#include "GlobalState.h"
#include "Logging.h"
#include "Battle.h"
#include "OptionsMenu.h"
#include "Persistence.h"
#include "ResourceStory.h"
#include "StoryList.h"
#include "TitleScreen.h"
#include "Utils.h"
#include "WorkerControl.h"

Window *baseWindow = NULL;

static bool hasFocus = true;

int HasFocus(void)
{
	return hasFocus;
}

Window *GetBaseWindow(void)
{
	return baseWindow;
}

void battery_state_handler(BatteryChargeState charge)
{
	UpdateBatteryLevel(charge);
}

// Called once per minute
void handle_time_tick(struct tm* tick_time, TimeUnits units_changed) 
{
	if(!hasFocus)
		return;
	
	UpdateGlobalState(units_changed);
		
	if(units_changed & MINUTE_UNIT)
	{
		UpdateClock();
	}
}

void focus_handler(bool in_focus) {
	hasFocus = in_focus;
	DEBUG_VERBOSE_LOG("Focus handler");
	if(hasFocus)
	{
		UpdateClock();
		SetUpdateDelay();
		INFO_LOG("Gained focus.");
	}
	else
	{
		INFO_LOG("Lost focus.");
	}
}

void handle_init() {
	
    ResourceStory_LoadAll();

    INFO_LOG("Starting MiniAdventure");
    GlobalState_Initialize();
    LoadGlobalPersistedData();
#if ALLOW_WORKER_APP
	if(WorkerIsRunning())
	{
#if ALLOW_WORKER_APP_LISTENING
		app_worker_message_subscribe(WorkerMessageHandler);
#endif
		AppAwake();
	}
#endif

	SeedRandom();
	DEBUG_LOG("Srand");
	
	handle_time_tick(NULL, SECOND_UNIT);
	DEBUG_LOG("First handle second");
	
	// Just here so that the health and level fields are always filled in.
	baseWindow = InitializeBaseWindow();
	DEBUG_LOG("push new window %p", baseWindow);
	window_stack_push(baseWindow, false);
	RegisterTitleScreen();
	tick_timer_service_subscribe(SECOND_UNIT, &handle_time_tick);
	app_focus_service_subscribe(focus_handler);
	battery_state_service_subscribe(battery_state_handler);
}

void handle_deinit()
{
	INFO_LOG("Cleaning up on exit.");
    SaveGlobalPersistedData();
    
    // This should only happen on a hard exit
    if(ResourceStory_InStory())
        AdventureScreenPop(NULL);
    
    if(ClosingWhileInBattle())
    {
        BattleScreenPop(NULL);
    }

#if ALLOW_WORKER_APP
	AppDying(ClosingWhileInBattle());
#endif
	tick_timer_service_unsubscribe();
	app_focus_service_unsubscribe();
	battery_state_service_unsubscribe();
#if ALLOW_WORKER_APP && ALLOW_WORKER_APP_LISTENING
	app_worker_message_unsubscribe();
#endif
	if(baseWindow)
		window_destroy(baseWindow);
    GlobalState_Free();
    ResourceBattler_UnloadPlayer();
    ResourceMonster_UnloadCurrent();
    ResourceStory_FreeAll();
}

// The main event/run loop for our app
int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}