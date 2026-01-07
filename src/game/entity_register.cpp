#include "entity_register.h"
#include "entity.h"
#include "example_entity.h"
#include "level.h"

void entity_register_all() {
    // Register all entity types here
    EntitySystem::register_entity_type("ExampleEntity", ExampleEntity::onregister, ExampleEntity::create);
    
}   