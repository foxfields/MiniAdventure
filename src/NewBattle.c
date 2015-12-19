#include <pebble.h>
#include "BattleActor.h"
#include "Character.h"
#include "DescriptionFrame.h"
#include "GlobalState.h"
#include "Logging.h"
#include "NewBattle.h"
#include "MainImage.h"
#include "Monsters.h"
#include "NewBaseWindow.h"
#include "NewMenu.h"
#include "Persistence.h"
#include "ProgressBar.h"
#include "Skills.h"
#include "Story.h"
#include "BattleQueue.h"

typedef struct NewBattleState
{
	uint16_t currentMonsterHealth;
	BattleActor *player;
	BattleActor *monster;
	int playerCurrentHealth;
	int playerMaxHealth;
	int monsterCurrentHealth;
	int monsterMaxHealth;
	int playerTime;
	int monsterTime;
} NewBattleState;

#if defined(PBL_ROUND)
static GRect playerHealthFrame = {.origin = {.x = 20, .y = 65}, .size = {.w = 16, .h = 50}};
static GRect playerTimeFrame = {.origin = {.x = 36, .y = 65}, .size = {.w = 8, .h = 50}};
static GRect monsterHealthFrame = {.origin = {.x = 148, .y = 65}, .size = {.w = 16, .h = 50}};
static GRect monsterTimeFrame = {.origin = {.x = 140, .y = 65}, .size = {.w = 8, .h = 50}};
#else
static GRect playerHealthFrame = {.origin = {.x = 20, .y = 65}, .size = {.w = 16, .h = 50}};
static GRect playerTimeFrame = {.origin = {.x = 36, .y = 65}, .size = {.w = 8, .h = 50}};
static GRect monsterHealthFrame = {.origin = {.x = 148, .y = 65}, .size = {.w = 16, .h = 50}};
static GRect monsterTimeFrame = {.origin = {.x = 140, .y = 65}, .size = {.w = 8, .h = 50}};
#endif


static ProgressBar *playerHealthBar;
static ProgressBar *playerTimeBar;

static ProgressBar *monsterHealthBar;
static ProgressBar *monsterTimeBar;

static int maxTimeCount = 100;

static bool battleCleanExit = true;

static NewBattleState gBattleState = {0};
static MonsterDef *currentMonster = NULL;

// in seconds
#define BATTLE_SAVE_TICK_DELAY 60
static int battleSaveTickDelay = 0;

void SaveBattleState(void)
{
	if(battleSaveTickDelay <= 0)
	{
		SavePersistedData();
		battleSaveTickDelay = BATTLE_SAVE_TICK_DELAY;
	}
}

int GetCurrentMonsterHealth(void)
{
	return gBattleState.currentMonsterHealth;
}

bool gUpdateNewBattle = false;
bool gPlayerTurn = false;
int gSkillDelay = 0;
const char *gEffectDescription = NULL;

void CloseNewBattleWindow(void)
{
	INFO_LOG("Ending battle.");
	battleCleanExit = true;
	ResetBattleQueue();
	PopGlobalState();
}

bool ClosingWhileInBattle(void)
{
	return !battleCleanExit;
}

BattleActor *GetPlayerActor(void)
{
	return gBattleState.player;
}

BattleActor *GetMonsterActor(void)
{
	return gBattleState.monster;
}

void PlayerPushFastAttack(void)
{
	SkillInstance *newInstance = CreateSkillInstance(GetFastAttack(), GetPlayerActor(), GetMonsterActor());
	BattleQueuePush(SKILL, newInstance);
	gPlayerTurn = false;
}

void PlayerPushSlowAttack(void)
{
	SkillInstance *newInstance = CreateSkillInstance(GetSlowAttack(), GetPlayerActor(), GetMonsterActor());
	BattleQueuePush(SKILL, newInstance);
	gPlayerTurn = false;
}

static uint16_t BattleScreenCount(void)
{
	DEBUG_LOG("BattleScreenCount");
	if(!gPlayerTurn)
		return 0;
	
	DEBUG_LOG("Returning good value");
	return 2;
}

static const char *BattleScreenNameCallback(int row)
{
	if(!gPlayerTurn)
		return NULL;

	switch(row)
	{
		case 0:
		{
			return "Fast Attack";
			break;
		}
		case 1:
		{
			return "Slow Attack";
			break;
		}
	}
	return "";
}

static const char *BattleScreenDescriptionCallback(int row)
{
	if(!gPlayerTurn)
		return NULL;

	switch(row)
	{
		case 0:
		{
			return "Quick stab";
			break;
		}
		case 1:
		{
			return "Heavy slash";
			break;
		}
	}
	return "";
}

static void BattleScreenSelectCallback(int row)
{
	if(!gPlayerTurn)
		return;

	switch(row)
	{
		case 0:
		{
			PlayerPushFastAttack();
			break;
		}
		case 1:
		{
			PlayerPushSlowAttack();
			break;
		}
	}
}

const char  *UpdateNewMonsterHealthText(void)
{
	static char monsterHealthText[] = "00000"; // Needs to be static because it's used by the system later.

	IntToString(monsterHealthText, 5, BattleActor_GetHealth(GetMonsterActor()));
	return monsterHealthText;
}

void BattleScreenAppear(void *data)
{
	SetDescription(currentMonster->name);
	RegisterMenuCellCallbacks(GetMainMenu(), BattleScreenCount, BattleScreenNameCallback, BattleScreenDescriptionCallback, BattleScreenSelectCallback);
	ReloadMenu(GetMainMenu());
}

int NewComputeMonsterHealth(int level)
{
	int baseHealth = 20 + ((level-1)*(level)/2) + ((level-1)*(level)*(level+1)/(6*2));
	return ScaleMonsterHealth(currentMonster, baseHealth);
}

static bool forcedBattle = false;
static int forcedBattleMonsterType = -1;
static int forcedBattleMonsterHealth = 0;
void ResumeBattle(int currentMonster, int currentMonsterHealth)
{
	if(currentMonster >= 0 && currentMonsterHealth > 0)
	{
		forcedBattle = true;
		forcedBattleMonsterType = currentMonster;
		forcedBattleMonsterHealth = currentMonsterHealth;
	}
}

bool IsBattleForced(void)
{
	return forcedBattle;
}

void NewBattleInit(void)
{
	currentMonster = NULL;

	if(forcedBattle)
	{
		DEBUG_LOG("Starting forced battle with (%d,%d)", forcedBattleMonsterType, forcedBattleMonsterHealth);
		currentMonster = GetFixedMonster(forcedBattleMonsterType);
		gBattleState.currentMonsterHealth = forcedBattleMonsterHealth;	
		forcedBattle = false;
	}

	if(!currentMonster)
	{
		currentMonster = GetRandomMonster();
		gBattleState.currentMonsterHealth = NewComputeMonsterHealth(GetCurrentBaseLevel());
	}
	CharacterData *character = GetCharacter();
	gBattleState.player = InitBattleActor(true, character->level, character->speed, character->stats.maxHealth);
	gBattleState.monster = InitBattleActor(false, GetCurrentBaseLevel(), currentMonster->speed, gBattleState.currentMonsterHealth);
	
	BattleQueuePush(ACTOR, GetPlayerActor());
	BattleQueuePush(ACTOR, GetMonsterActor());

	playerHealthBar = CreateProgressBar(&gBattleState.playerCurrentHealth, &gBattleState.playerMaxHealth, FILL_UP, playerHealthFrame, GColorBrightGreen, -1);
	monsterHealthBar = CreateProgressBar(&gBattleState.monsterCurrentHealth, &gBattleState.monsterMaxHealth, FILL_UP, monsterHealthFrame, GColorFolly, -1);
	playerTimeBar = CreateProgressBar(&gBattleState.playerTime, &maxTimeCount, FILL_UP, playerTimeFrame, GColorVeryLightBlue, -1);
	monsterTimeBar = CreateProgressBar(&gBattleState.monsterTime, &maxTimeCount, FILL_UP, monsterTimeFrame, GColorRichBrilliantLavender, -1);

	InitializeProgressBar(playerHealthBar, GetBaseWindow());
	InitializeProgressBar(playerTimeBar, GetBaseWindow());
	InitializeProgressBar(monsterHealthBar, GetBaseWindow());
	InitializeProgressBar(monsterTimeBar, GetBaseWindow());

	// Force the main menu to the front
	InitializeNewMenuLayer(GetMainMenu(), GetBaseWindow());
	
	RegisterMenuCellCallbacks(GetMainMenu(), BattleScreenCount, BattleScreenNameCallback, BattleScreenDescriptionCallback, BattleScreenSelectCallback);

	SetForegroundImage(currentMonster->imageId);
#if defined(PBL_COLOR)
	SetBackgroundImage(RESOURCE_ID_IMAGE_BATTLE_FLOOR);
#endif
	SetMainImageVisibility(true, true, true);
	battleCleanExit = false;
}

void UpdateProgressBars(void)
{
	gBattleState.playerCurrentHealth = BattleActor_GetHealth(GetPlayerActor());
	gBattleState.playerMaxHealth = BattleActor_GetMaxHealth(GetPlayerActor());
	gBattleState.monsterCurrentHealth = BattleActor_GetHealth(GetMonsterActor());
	gBattleState.monsterMaxHealth = BattleActor_GetMaxHealth(GetMonsterActor());
	
	gBattleState.playerTime = GetCurrentTimeInQueue(true);
	gBattleState.monsterTime = GetCurrentTimeInQueue(false);
}

void UpdateNewBattle(void *unused)
{
	bool entryRemoved = false;
	void *data = NULL;
	BattleQueueEntryType type = ACTOR;

	if(battleSaveTickDelay > 0)
	{
		--battleSaveTickDelay;
	}
	
	if(gSkillDelay > 0)
	{
		--gSkillDelay;
		if(gSkillDelay == 0)
			SetDescription(currentMonster->name);
		return;
	}
	
	if(gPlayerTurn)
		return;
	
	if(BattleActor_GetHealth(gBattleState.player) <= 0 || BattleActor_GetHealth(gBattleState.monster) <= 0)
	{
		CloseNewBattleWindow();
		return;
	}
	
	
	entryRemoved = UpdateBattleQueue(&type, &data);
	
	UpdateProgressBars();
	MarkProgressBarDirty(playerTimeBar);
	MarkProgressBarDirty(monsterTimeBar);

	if(entryRemoved)
	{
		switch(type)
		{
			case SKILL:
			{
				SkillInstance *instance = (SkillInstance*)data;
				gEffectDescription = ExecuteSkill(instance);
				UpdateProgressBars();
				MarkProgressBarDirty(playerHealthBar);
				MarkProgressBarDirty(monsterHealthBar);
				SetDescription(gEffectDescription);
				BattleQueuePush(ACTOR, SkillInstanceGetAttacker(instance));
				gSkillDelay = SKILL_DELAY;
				break;
			}
			case ACTOR:
			{
				BattleActor *actor = (BattleActor*)data;
				if(BattleActor_IsPlayer(actor))
				{
					gPlayerTurn = true;
					ReloadMenu(GetMainMenu());
				}
				else
				{
					// Fake monster AI
					SkillInstance *newInstance = CreateSkillInstance(GetFastAttack(), GetMonsterActor(), GetPlayerActor());
					BattleQueuePush(SKILL, newInstance);
				}
				break;
			}
		}
	}
}

void BattleScreenPush(void *data)
{
	NewBattleInit();
	HideBatteryLevel();
}

void BattleScreenPop(void *data)
{
	ShowBatteryLevel();
	RemoveProgressBar(playerHealthBar);
	RemoveProgressBar(playerTimeBar);
	RemoveProgressBar(monsterHealthBar);
	RemoveProgressBar(monsterTimeBar);
	FreeProgressBar(playerHealthBar);
	FreeProgressBar(playerTimeBar);
	FreeProgressBar(monsterHealthBar);
	FreeProgressBar(monsterTimeBar);
}

void TriggerBattleScreen(void)
{
	if(CurrentLocationAllowsCombat())
		PushGlobalState(STATE_BATTLE, SECOND_UNIT, UpdateNewBattle, BattleScreenPush, BattleScreenAppear, NULL, BattleScreenPop, NULL);
}
