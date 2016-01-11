#include "pebble.h"

#include "Adventure.h"
#include "Character.h"
#include "CombatantClass.h"
#include "DialogFrame.h"
#include "Logging.h"
#include "ResourceStory.h"
#include "Utils.h"

typedef struct Character
{
    uint16_t classType;
    int level;
    int currentHealth;
    uint16_t currentXP;
    uint16_t skillCooldowns[MAX_SKILLS_IN_LIST];
} Character;

Character character =
{
    .classType = 0,
    .level = 1,
    .currentHealth = 1,
    .currentXP = 0,
    .skillCooldowns = {0}
};

void Character_SetCooldowns(uint16_t *cooldowns)
{
    for(int i = 0; i < MAX_SKILLS_IN_LIST; ++i)
    {
        character.skillCooldowns[i] = cooldowns[i];
    }
}

uint16_t *Character_GetCooldowns(void)
{
    return character.skillCooldowns;
}

void Character_SetClass(int type)
{
    character.classType = type;
}

int Character_GetHealth(void)
{
    return character.currentHealth;
}

void Character_SetHealth(int health)
{
    character.currentHealth = health;
}

int Character_GetLevel(void)
{
    return character.level;
}

Character *GetCharacter(void)
{
    return &character;
}

size_t Character_GetDataSize(void)
{
	return sizeof(Character);
}

void Character_Rest(void)
{
    character.currentHealth = CombatantClass_GetHealth(&BattlerWrapper_GetPlayerWrapper()->battler.combatantClass, character.level);
    
    for(int i = 0; i < MAX_SKILLS_IN_LIST; ++i)
    {
        character.skillCooldowns[i] = 0;
    }
   
}

void Character_GrantLevel(void)
{
    character.level++;
    character.currentHealth = CombatantClass_GetHealth(&BattlerWrapper_GetPlayerWrapper()->battler.combatantClass, character.level);
}

void Character_GrantXP(uint16_t monsterLevel)
{
    uint16_t xpMonstersPerLevel = ResourceStory_GetCurrentStoryXPMonstersPerLevel();
    uint16_t xpDifferenceScale = ResourceStory_GetCurrentStoryXPDifferenceScale();
    
    if(xpMonstersPerLevel == 0)
        return;
    
    // We add 1 to work around truncation issues.
    uint16_t xpGain = (XP_TO_LEVEL_UP * (100 + (monsterLevel - character.level) * xpDifferenceScale)) / (xpMonstersPerLevel * 100) + 1;
    
    character.currentXP += xpGain;
    while(character.currentXP >= XP_TO_LEVEL_UP)
    {
        character.level++;
        character.currentXP -= XP_TO_LEVEL_UP;
        character.currentHealth = CombatantClass_GetHealth(&BattlerWrapper_GetPlayerWrapper()->battler.combatantClass, character.level);
    }
}

void Character_ReadPersistedData(int index)
{
	persist_read_data(index, &character, sizeof(Character));	
    ResourceBattler_LoadPlayer(character.classType);
}

void Character_WritePersistedData(int index)
{
	persist_write_data(index, &character, sizeof(Character));
}

void Character_Initialize(void)
{
    INFO_LOG("Initializing character.");
    character.classType = 0; // Get the first class in the story
    ResourceBattler_LoadPlayer(character.classType);
    character.level = 1;
    character.currentXP = 0;
    character.currentHealth = CombatantClass_GetHealth(&BattlerWrapper_GetPlayerWrapper()->battler.combatantClass, character.level);
}

char RankToCharacter(int rank)
{
    if(rank == 6)
        return 'S';
    else
        return 'F' - rank;
}

void Character_ShowClass(void)
{
    DialogData *dialog = calloc(sizeof(DialogData), 1);
    char *text;
    CombatantClass *combatant = &BattlerWrapper_GetPlayerWrapper()->battler.combatantClass;
    dialog->heap = true;
    dialog->allowCancel = false;
    text = calloc(sizeof(char), 256);
    snprintf(text, 256, "Class: %s\n\nStrength: %c\nMagic: %c\nDefense: %c\nMagDefense: %c\nSpeed: %c\nHealth: %c\n", BattlerWrapper_GetPlayerWrapper()->battler.name, RankToCharacter(combatant->strengthRank),
             RankToCharacter(combatant->magicRank),
             RankToCharacter(combatant->defenseRank),
             RankToCharacter(combatant->magicDefenseRank),
             RankToCharacter(combatant->speedRank),
             RankToCharacter(combatant->healthRank));
    dialog->text = text;
    QueueDialog(dialog);
}

void Character_ShowSkills(void)
{
    DialogData *dialog = calloc(sizeof(DialogData), 1);
    char *text;
    SkillList *skillList = &BattlerWrapper_GetPlayerWrapper()->battler.skillList;
    dialog->heap = true;
    dialog->allowCancel = false;
    text = calloc(sizeof(char), 256);
    snprintf(text, 256, "Skill: cooldown\n");
    for(int i = 0; i < skillList->count; ++i)
    {
        if(skillList->entries[i].level > character.level)
            break;
        char skillInfo[20] = {0};
        Skill *skill = BattlerWrapper_GetSkillByIndex(BattlerWrapper_GetPlayerWrapper(), i);
        if(skill->cooldown == 0)
        {
            snprintf(skillInfo, 20, "%s: None\n", skill->name);
        }
        else
        {
            snprintf(skillInfo, 20, "%s: %d/%d\n", skill->name, character.skillCooldowns[i],skill->cooldown);
        }
        strncat(text, skillInfo, 20);
    }
    
    dialog->text = text;
    QueueDialog(dialog);
}

void Character_ShowStatus(void)
{
    DialogData *dialog = calloc(sizeof(DialogData), 1);
    char *text;
    CombatantClass *combatant = &BattlerWrapper_GetPlayerWrapper()->battler.combatantClass;
    dialog->heap = true;
    dialog->allowCancel = false;
    text = calloc(sizeof(char), 256);
    snprintf(text, 256, "Status\n\nLevel: %d\nXP: %d/%d, Health: %d/%d", character.level, character.currentXP, XP_TO_LEVEL_UP, character.currentHealth, CombatantClass_GetHealth(combatant, character.level));
    
    dialog->text = text;
    QueueDialog(dialog);
}
