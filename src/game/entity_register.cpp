#include "entity_register.h"
#include "entity.h"
#include "example_entity.h"
#include "journal_entry.h"
#include "item_instance.h"
#include "sliding_door.h"
#include "interactive_model.h"
#include "trigger_zone.h"
#include "level.h"

void entity_register_all() {
    // Register all entity types here
    EntitySystem::register_entity_type(EXAMPLE_ENTITY_TYPE_NAME, ExampleEntity::create);
    EntitySystem::register_entity_type(JOURNAL_ENTRY_TYPE_NAME, JournalEntry::create); JournalEntry::preinit();
    EntitySystem::register_entity_type(ITEM_INSTANCE_TYPE_NAME, ItemInstance::create); ItemInstance::preinit();
    
    EntitySystem::register_entity_type(SLIDING_DOOR_TYPE_NAME, SlidingDoor::create);
    EntitySystem::register_entity_type(INTERACTIVE_MODEL_TYPE_NAME, InteractiveModel::create);
    EntitySystem::register_entity_type(TRIGGER_ZONE_TYPE_NAME, TriggerZone::create);
}   
