#pragma once

#include "AutoSizeConstants.h"

typedef struct Skill Skill;
typedef struct SkillList SkillList;
typedef struct BattlerWrapper BattlerWrapper;

typedef enum
{
    STORYUPDATE_COMPUTERANDOM = 0,
    STORYUPDATE_DONOTHING,
    STORYUPDATE_FULLREFRESH,
} ResourceStoryUpdateReturnType;

void ResourceStory_InitializeCurrent(void);

uint16_t ResourceStory_GetCurrentLocationIndex(void);
int ResourceStory_GetCurrentLocationBackgroundImageId(void);
bool ResourceStory_IncrementTimeOnPath(void);
uint16_t ResourceStory_GetTimeOnPath(void);
uint16_t ResourceStory_GetCurrentLocationLength(void);
bool ResourceStory_CurrentLocationIsPath(void);
ResourceStoryUpdateReturnType ResourceStory_UpdateCurrentLocation(void);
const char *ResourceStory_GetCurrentLocationName(void);
uint16_t ResourceStory_GetCurrentAdjacentLocations(void);
ResourceStoryUpdateReturnType ResourceStory_MoveToLocation(uint16_t index);
const char *ResourceStory_GetAdjacentLocationName(uint16_t index);
uint16_t ResourceStory_GetCurrentLocationLength(void);
bool ResourceStory_CurrentLocationIsPath(void);
uint16_t ResourceStory_GetCurrentLocationBaseLevel(void);
uint16_t ResourceStory_GetCurrentLocationEncounterChance(void);
bool ResourceStory_InStory(void);

uint16_t BattlerWrapper_GetUsableSkillCount(BattlerWrapper *wrapper, uint16_t level);
Skill *BattlerWrapper_GetSkillByIndex(BattlerWrapper *wrapper, uint16_t index);
CombatantClass *BattlerWrapper_GetCombatantClass(BattlerWrapper *wrapper);
BattlerWrapper *BattlerWrapper_GetPlayerWrapper(void);
BattlerWrapper *BattlerWrapper_GetMonsterWrapper(void);
const char *BattlerWrapper_GetName(BattlerWrapper *wrapper);
int BattlerWrapper_GetImage(BattlerWrapper *wrapper);

void ResourceStory_LoadAll(void);
void ResourceStory_LogCurrent(void);
void ResourceStory_FreeAll(void);
void ResourceStory_SetCurrentStory(uint16_t index);
void ResourceStory_ClearCurrentStory(void);

const char *ResourceStory_GetNameByIndex(uint16_t index);
const char *ResourceStory_GetDescriptionByIndex(uint16_t index);

void ResourceStory_GetStoryList(uint16_t *count, uint16_t **buffer);
uint16_t ResourceStory_GetCurrentStoryId(void);
uint16_t ResourceStory_GetCurrentStoryVersion(void);
void ResourceStory_GetPersistedData(uint16_t *count, uint8_t **buffer);
void ResourceStory_UpdateStoryWithPersistedState(void);

bool ResourceStory_CurrentLocationHasMonster(void);
int ResourceStory_GetCurrentLocationMonster(void);
void ResourceBattler_LoadPlayer(uint16_t classId);

char *ResourceMonster_GetCurrentName(void);
void ResourceMonster_UnloadCurrent(void);
void ResourceMonster_LoadCurrent(uint16_t index);
bool ResourceMonster_Loaded(void);
void ResourceBattler_UnloadPlayer(void);
void ResourceMonster_UnloadCurrent(void);

