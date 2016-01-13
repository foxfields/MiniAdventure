#include <pebble.h>
#include "AutoSizeConstants.h"
#include "BaseWindow.h"
#include "Battle.h"
#include "BinaryResourceLoading.h"
#include "Character.h"
#include "Clock.h"
#include "CombatantClass.h"
#include "DescriptionFrame.h"
#include "DialogFrame.h"
#include "EngineInfo.h"
#include "ExtraMenu.h"
#include "GlobalState.h"
#include "Logging.h"
#include "MainImage.h"
#include "Menu.h"
#include "Persistence.h"
#include "ProgressBar.h"
#include "ResourceStory.h"
#include "Skills.h"

typedef struct BattleState
{
    BattleActorWrapper player;
    BattleActorWrapper monster;
} BattleState;

#if defined(PBL_ROUND)
#define PLAYER_HEALTH_FRAME {.origin = {.x = 50, .y = 90}, .size = {.w = 16, .h = 40}}
#define PLAYER_TIME_FRAME  {.origin = {.x = 67, .y = 90}, .size = {.w = 8, .h = 40}}
#define MONSTER_HEALTH_FRAME {.origin = {.x = 50, .y = 48}, .size = {.w = 16, .h = 40}}
#define MONSTER_TIME_FRAME {.origin = {.x = 67, .y = 48}, .size = {.w = 8, .h = 40}}
#else
#define PLAYER_HEALTH_FRAME {.origin = {.x = 20, .y = 65}, .size = {.w = 16, .h = 40}}
#define PLAYER_TIME_FRAME  {.origin = {.x = 36, .y = 65}, .size = {.w = 8, .h = 40}}
#define MONSTER_HEALTH_FRAME {.origin = {.x = 148, .y = 65}, .size = {.w = 16, .h = 40}}
#define MONSTER_TIME_FRAME {.origin = {.x = 140, .y = 65}, .size = {.w = 8, .h = 40}}
#endif

static ProgressBar *playerHealthBar = NULL;
static ProgressBar *playerTimeBar = NULL;

static ProgressBar *monsterHealthBar = NULL;
static ProgressBar *monsterTimeBar = NULL;

static uint16_t maxTimeCount = 100;

static bool battleCleanExit = true;

static BattleState gBattleState =
{
    .player =
    {
        .actor = {0},
        .battlerWrapper = NULL
    },
    .monster =
    {
        .actor = {0},
        .battlerWrapper = NULL
    }
};

static uint16_t currentMonsterIndex = 0;

uint16_t Battle_GetCurrentMonsterIndex(void)
{
    return currentMonsterIndex;
}

// in seconds
#define BATTLE_SAVE_TICK_DELAY 60
static int battleSaveTickDelay = 0;

void SaveBattleState(void)
{
    if(battleSaveTickDelay <= 0)
    {
        //SavePersistedData();
        battleSaveTickDelay = BATTLE_SAVE_TICK_DELAY;
    }
}

void Battle_WritePlayerData(int index)
{
    persist_write_data(index, &gBattleState.player.actor, sizeof(BattleActor));
}

void Battle_WriteMonsterData(int index)
{
    persist_write_data(index, &gBattleState.monster.actor, sizeof(BattleActor));
}

void Battle_ReadPlayerData(int index)
{
    persist_read_data(index, &gBattleState.player.actor, sizeof(BattleActor));
}

void Battle_ReadMonsterData(int index)
{
    persist_read_data(index, &gBattleState.monster.actor, sizeof(BattleActor));
}

static bool gPlayerTurn = false;
static bool gPlayerActed = false;
static int gSkillDelay = 0;
static const char *gEffectDescription = NULL;

void CloseBattleWindow(void)
{
    INFO_LOG("Ending battle.");
    battleCleanExit = true;
    GlobalState_Pop();
    Character_SetHealth(gBattleState.player.actor.currentHealth);
    if(gBattleState.player.actor.currentHealth > 0)
    {
        // The player won, so grant xp.
        Character_GrantXP(gBattleState.monster.actor.level);
    }
    
    Character_SetCooldowns(gBattleState.player.actor.skillCooldowns);
}

bool ClosingWhileInBattle(void)
{
    return !battleCleanExit;
}

void Battle_SetCleanExit(void)
{
    battleCleanExit = true;
}

uint16_t BattleScreen_MenuSectionCount(void)
{
    return 2 + ExtraMenu_GetSectionCount();
}

const char *BattleScreen_MenuSectionName(uint16_t sectionIndex)
{
    switch(sectionIndex)
    {
        case 0:
            return "Skills";
        case 1:
            return "Battle";
        case 2:
            return ExtraMenu_GetSectionName();
    }
    return "None";
}

uint16_t BattleScreen_MenuCellCount(uint16_t sectionIndex)
{
    switch(sectionIndex)
    {
        case 0:
        {
            if(!gPlayerTurn)
                return 0;
            
            return BattlerWrapper_GetUsableSkillCount(gBattleState.player.battlerWrapper, gBattleState.player.actor.level);
        }
        case 1:
        {
            return 4;
        }
        case 2:
        {
            return ExtraMenu_GetCellCount();
        }
    }
    return 0;
}

const char *BattleScreen_MenuCellName(MenuIndex *index)
{
    switch(index->section)
    {
        case 0:
        {
            if(!gPlayerTurn)
                return NULL;
            
            Skill *skill = BattlerWrapper_GetSkillByIndex(gBattleState.player.battlerWrapper, index->row);
            return skill->name;
        }
        case 1:
        {
            switch(index->row)
            {
                case 0:
                    return "Status";
                case 1:
                    return "Class";
                case 2:
                    return "Skills";
                case 3:
                    return "Reset";
            }
            break;
        }
        case 2:
        {
            return ExtraMenu_GetCellName(index->row);
        }
    }
    return "None";
}

const char *BattleScreen_MenuCellDescription(MenuIndex *index)
{
    switch(index->section)
    {
        case 0:
        {
            if(!gPlayerTurn)
                return NULL;
            
            if(gBattleState.player.actor.skillCooldowns[index->row] > 0)
                return "On cooldown";
            Skill *skill = BattlerWrapper_GetSkillByIndex(gBattleState.player.battlerWrapper, index->row);
            return skill->name;
        }
        case 1:
        {
            switch(index->row)
            {
                case 0:
                    return "Status";
                case 1:
                    return "Class";
                case 2:
                    return "Skills";
                case 3:
                    return "Reset";
            }
            break;
        }
        case 2:
        {
            return ExtraMenu_GetCellName(index->row);
        }
    }
    return "None";
}

void BattleScreen_MenuSelect(MenuIndex *index)
{
    switch(index->section)
    {
        case 0:
        {
            if(!gPlayerTurn)
                return;
            
            if(gBattleState.player.actor.skillCooldowns[index->row] > 0)
                return;
            gBattleState.player.actor.skillQueued = true ;
            gBattleState.player.actor.activeSkill = index->row;
            gPlayerActed = true;
            break;
        }
        case 1:
        {
            switch(index->row)
            {
                case 0:
                {
                    Character_ShowStatus();
                    break;
                }
                case 1:
                {
                    Character_ShowClass();
                    break;
                }
                case 2:
                {
                    Character_ShowSkills();
                    break;
                }
                case 3:
                {
                    DialogData *dialog = calloc(sizeof(DialogData), 1);
                    ResourceLoadStruct(EngineInfo_GetResHandle(), EngineInfo_GetInfo()->resetPromptDialog, (uint8_t*)dialog, sizeof(DialogData), "DialogData");
                    dialog->allowCancel = true;
                    QueueDialog(dialog);
                    GlobalState_QueueStatePop();
                    GlobalState_Queue(STATE_RESET_GAME, 0, NULL);
                    break;
                }
            }
            break;
        }
        case 2:
        {
            ExtraMenu_SelectAction(index->row);
        }
    }
}

void BattleScreenAppear(void *data)
{
    if(gPlayerActed)
    {
        gPlayerTurn = false;
        gPlayerActed = false;
        gBattleState.player.actor.currentTime = 0;
    }
    SetDescription(ResourceMonster_GetCurrentName());
    RegisterMenuState(GetMainMenu(), STATE_BATTLE);
    RegisterMenuState(GetSlaveMenu(), STATE_NONE);
    SetForegroundImage(gBattleState.monster.battlerWrapper->battler.image);
#if defined(PBL_COLOR)
    SetBackgroundImage(RESOURCE_ID_IMAGE_BATTLEFLOOR);
#endif
    SetMainImageVisibility(true, true, true);
}

static bool forcedBattle = false;
static int forcedBattleMonsterType = -1;
void ResumeBattle(int currentMonster)
{
    forcedBattle = true;
    forcedBattleMonsterType = currentMonster;
}

void ForceRandomBattle(void)
{
    forcedBattle = true;
    forcedBattleMonsterType = -1;
}

bool IsBattleForced(void)
{
    return forcedBattle;
}

static void InitializeBattleActorWrapper(BattleActorWrapper *actorWrapper, BattlerWrapper *battlerWrapper,uint16_t level, uint16_t currentHealth, uint16_t *skillCooldowns)
{
    actorWrapper->battlerWrapper = battlerWrapper;
    actorWrapper->actor.level = level;
    actorWrapper->actor.maxHealth = CombatantClass_GetHealth(&battlerWrapper->battler.combatantClass, level);
    actorWrapper->actor.skillQueued = false;
    actorWrapper->actor.activeSkill = INVALID_SKILL;
    actorWrapper->actor.counterSkill = INVALID_SKILL;
    actorWrapper->actor.currentTime = 0;
    if(currentHealth == 0)
        actorWrapper->actor.currentHealth = actorWrapper->actor.maxHealth;
    else
        actorWrapper->actor.currentHealth = currentHealth;
    for(int i = 0; i < MAX_SKILLS_IN_LIST; ++i)
    {
        actorWrapper->actor.skillCooldowns[i] = skillCooldowns[i];
    }
}

void BattleInit(void)
{
    ResourceMonster_UnloadCurrent();
    
    if(forcedBattle && forcedBattleMonsterType > -1)
    {
        ResourceMonster_LoadCurrent(forcedBattleMonsterType);
        gBattleState.monster.battlerWrapper = BattlerWrapper_GetMonsterWrapper();
        gBattleState.player.battlerWrapper = BattlerWrapper_GetPlayerWrapper();
        currentMonsterIndex = forcedBattleMonsterType;
        forcedBattle = false;
    }
    else
    {
        forcedBattle = false;
        if(!ResourceMonster_Loaded())
        {
            currentMonsterIndex = ResourceStory_GetCurrentLocationMonster();
            ResourceMonster_LoadCurrent(currentMonsterIndex);
        }
        InitializeBattleActorWrapper(&gBattleState.player, BattlerWrapper_GetPlayerWrapper(), Character_GetLevel(), Character_GetHealth(), Character_GetCooldowns());
        uint16_t skillCooldowns[MAX_SKILLS_IN_LIST] = {0};
        InitializeBattleActorWrapper(&gBattleState.monster, BattlerWrapper_GetMonsterWrapper(), ResourceStory_GetCurrentLocationBaseLevel(), 0, skillCooldowns);
    }
    
    GRect playerHealthFrame = PLAYER_HEALTH_FRAME;
    GRect playerTimeFrame = PLAYER_TIME_FRAME;
    GRect monsterHealthFrame = MONSTER_HEALTH_FRAME;
    GRect monsterTimeFrame = MONSTER_TIME_FRAME;
    
    playerHealthBar = CreateProgressBar(&gBattleState.player.actor.currentHealth, &gBattleState.player.actor.maxHealth, FILL_UP, &playerHealthFrame, GColorBrightGreen, -1);
    monsterHealthBar = CreateProgressBar(&gBattleState.monster.actor.currentHealth, &gBattleState.monster.actor.maxHealth, FILL_DOWN, &monsterHealthFrame, GColorFolly, -1);
    playerTimeBar = CreateProgressBar(&gBattleState.player.actor.currentTime, &maxTimeCount, FILL_UP, &playerTimeFrame, GColorVeryLightBlue, -1);
    monsterTimeBar = CreateProgressBar(&gBattleState.monster.actor.currentTime, &maxTimeCount, FILL_DOWN, &monsterTimeFrame, GColorRichBrilliantLavender, -1);
    
    InitializeProgressBar(playerHealthBar, GetBaseWindow());
    InitializeProgressBar(playerTimeBar, GetBaseWindow());
    InitializeProgressBar(monsterHealthBar, GetBaseWindow());
    InitializeProgressBar(monsterTimeBar, GetBaseWindow());
    
    HideDateLayer();
    
    // Force the main menu to the front
    InitializeMenuLayer(GetMainMenu(), GetBaseWindow());
    InitializeMenuLayer(GetSlaveMenu(), GetBaseWindow());
    
    // Force dialog layer to the top
    InitializeDialogLayer(GetBaseWindow());

    RegisterMenuState(GetMainMenu(), STATE_BATTLE);
    RegisterMenuState(GetSlaveMenu(), STATE_NONE);
    
    DEBUG_VERBOSE_LOG("Finished battle init");
    battleCleanExit = false;
}

static void DoSkill(Skill *skill, BattleActorWrapper *attacker, BattleActorWrapper *defender)
{
    gEffectDescription = ExecuteSkill(skill, attacker, defender);
    MarkProgressBarDirty(playerHealthBar);
    MarkProgressBarDirty(monsterHealthBar);
    SetDescription(gEffectDescription);
    gSkillDelay = SKILL_DELAY;
    attacker->actor.currentTime = 0;
    MarkProgressBarDirty(playerTimeBar);
    MarkProgressBarDirty(monsterTimeBar);
    attacker->actor.skillQueued = false;
}

static void UpdateActorCurrentTime(BattleActorWrapper *wrapper)
{
    if(wrapper->actor.skillQueued)
    {
        wrapper->actor.currentTime += BattlerWrapper_GetSkillByIndex(wrapper->battlerWrapper, wrapper->actor.activeSkill)->speed;
    }
    else
    {
        wrapper->actor.currentTime += CombatantClass_GetSpeed(&wrapper->battlerWrapper->battler.combatantClass, wrapper->actor.level);
    }
}

void UpdateBattle(void *unused)
{
    if(battleSaveTickDelay > 0)
    {
        --battleSaveTickDelay;
    }
    
    if(gSkillDelay > 0)
    {
        --gSkillDelay;
        if(gSkillDelay == 0)
            SetDescription(ResourceMonster_GetCurrentName());
        return;
    }
    
    if(gPlayerTurn)
        return;
    
    if(gBattleState.player.actor.currentHealth <= 0 || gBattleState.monster.actor.currentHealth <= 0)
    {
        CloseBattleWindow();
        return;
    }
    
    bool playerAhead = gBattleState.player.actor.currentTime >= gBattleState.monster.actor.currentTime;
    bool actionPerformed = false;
    
    if(playerAhead)
    {
        if(gBattleState.player.actor.currentTime >= maxTimeCount)
        {
            if(gBattleState.player.actor.skillQueued)
            {
                Skill *skill = BattlerWrapper_GetSkillByIndex(gBattleState.player.battlerWrapper, gBattleState.player.actor.activeSkill);
                DoSkill(skill, &gBattleState.player, &gBattleState.monster);
            }
            else
            {
                UpdateSkillCooldowns(gBattleState.player.actor.skillCooldowns);
                gPlayerTurn = true;
                gPlayerActed = false;
                RegisterMenuState(GetMainMenu(), STATE_BATTLE);
                RegisterMenuState(GetSlaveMenu(), STATE_NONE);
                Menu_ResetSelection(GetMainMenu());
                SetDescription("Your turn");
            }
            actionPerformed = true;
        }
    }
    else
    {
        if(gBattleState.monster.actor.currentTime >= maxTimeCount)
        {
            if(gBattleState.monster.actor.skillQueued)
            {
                Skill *skill = BattlerWrapper_GetSkillByIndex(gBattleState.monster.battlerWrapper, gBattleState.monster.actor.activeSkill);
                DoSkill(skill, &gBattleState.monster, &gBattleState.player);
            }
            else
            {
                UpdateSkillCooldowns(gBattleState.monster.actor.skillCooldowns);
                gBattleState.monster.actor.activeSkill = 0;
                gBattleState.monster.actor.skillQueued = true;
                gBattleState.monster.actor.currentTime = 0;
            }
            actionPerformed = true;
        }
    }

    if(!actionPerformed)
    {
        UpdateActorCurrentTime(&gBattleState.player);
        UpdateActorCurrentTime(&gBattleState.monster);

        MarkProgressBarDirty(playerTimeBar);
        MarkProgressBarDirty(monsterTimeBar);
    }
}

void BattleScreenPush(void *data)
{
    BattleInit();
}

void BattleScreenPop(void *data)
{
    RemoveProgressBar(playerHealthBar);
    RemoveProgressBar(playerTimeBar);
    RemoveProgressBar(monsterHealthBar);
    RemoveProgressBar(monsterTimeBar);
    FreeProgressBar(playerHealthBar);
    FreeProgressBar(playerTimeBar);
    FreeProgressBar(monsterHealthBar);
    FreeProgressBar(monsterTimeBar);
    ResourceMonster_UnloadCurrent();
}

void TriggerBattleScreen(void)
{
    if(ResourceStory_CurrentLocationHasMonster())
        GlobalState_Push(STATE_BATTLE, SECOND_UNIT, NULL);
}
