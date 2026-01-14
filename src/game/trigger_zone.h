#ifndef TRIGGER_ZONE_H
#define TRIGGER_ZONE_H

#include "entity.h"

#define TRIGGER_ZONE_TYPE_NAME "TriggerZone"

#define TRIGGER_ZONE_TRIGGERED_BIT (1<<3)

class TriggerZone : public Entity {
private:
    std::string on_trigger_command;
    bool one_shot;
    bool triggered;

    bool check_collision();

public:
    TriggerZone(const std::string& name, int id);
    virtual ~TriggerZone() = default;

    void load_from_ini(const tortellini::ini::section& section) override;
    void load_from_eeprom(uint16_t data) override;
    uint16_t save_to_eeprom() const override;
    void init() override;
    void update() override;
    void draw() override;

    static Entity* create(const std::string& name, int id);
};

#endif
