#ifndef ITEM_INSTANCE_H
#define ITEM_INSTANCE_H

#include "entity.h"
#include "../engine_t3dm_wrapper.h"

#define ITEM_INSTANCE_TYPE_NAME ("ItemInstance")

#define ITEM_INSTANCE_PICKEDUP_BIT (1<<3)
#define MAX_ITEMS 256

extern tortellini::ini itemsdict;
extern std::string items_names[MAX_ITEMS];

const tortellini::ini::section& get_item_info_by_id(int id);
int items_check_combine_recepies(uint8_t item1_id, uint8_t item2_id);

// Example entity class
class ItemInstance : public Entity {
private:
    int item_id;
    bool pickedup;
    T3DMWModel itemmodel;
    std::string requireditemtopickup;
    float pickuprange;

public:
    ItemInstance(const std::string& name, int id);
    virtual ~ItemInstance() = default;

    // Implement required virtual functions
    virtual void load_from_ini(const tortellini::ini::section& section) override;
    virtual void load_from_eeprom(uint16_t data) override;
    virtual uint16_t save_to_eeprom() const override;
    virtual void init() override;
    virtual void update() override;
    virtual void draw() override;

    static void preinit();

    // Factory function
    static Entity* create(const std::string& name, int id);
};

#endif

