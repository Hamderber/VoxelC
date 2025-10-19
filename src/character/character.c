#include <string.h>
#include <stdlib.h>
#include "core/logs.h"
#include "character/character.h"
#include "character/characterType_t.h"
#include "world/worldState_t.h"
#include "character/characterController.h"

Character_t *character_create(State_t *state, CharacterType_t characterType)
{
    Character_t *character = calloc(1, sizeof(Character_t));
    character->type = characterType;

    switch (characterType)
    {
    case CHARACTER_TYPE_PLAYER:
        character->name = "Player"; // string literal (read-only)
        character->nameLength = strlen(character->name);
        logs_log(LOG_DEBUG, "Created player character '%s'", character->name);
        character_init(state, character);
        break;
    case CHARACTER_TYPE_MOB:
    default:
        logs_log(LOG_WARN, "Character type not yet implemented!");
        break;
    }

    return character;
}