#ifndef EXAMPLE_ENTITY_H
#define EXAMPLE_ENTITY_H
/// Example entity class demonstrating how to create one

#include "entity.h"

#define EXAMPLEENTITY_TYPE_NAME ("ExampleEntity")

// Example entity class
class ExampleEntity : public Entity {
private:
    // Example custom data
    float custom_value;

public:
    ExampleEntity(const std::string& name, int id);
    virtual ~ExampleEntity() = default;

    // Implement required virtual functions
    virtual void load_from_ini(const tortellini::ini::section& section) override;
    virtual void load_from_eeprom(uint16_t data) override;
    virtual uint16_t save_to_eeprom() const override;
    virtual void init() override;
    virtual void update() override;
    virtual void draw() override;

    // Factory function
    static Entity* create(const std::string& name, int id);
};

#endif

