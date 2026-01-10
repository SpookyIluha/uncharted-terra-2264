#include "entity_register.h"
#include "entity.h"
#include "example_entity.h"
#include "journal_entry.h"
#include "item_instance.h"
#include "level.h"

void entity_register_all() {
    // Register all entity types here
    EntitySystem::register_entity_type(EXAMPLE_ENTITY_TYPE_NAME, ExampleEntity::create);
    EntitySystem::register_entity_type(JOURNAL_ENTRY_TYPE_NAME, JournalEntry::create); JournalEntry::preinit();
    EntitySystem::register_entity_type(ITEM_INSTANCE_TYPE_NAME, ItemInstance::create); ItemInstance::preinit();
}   