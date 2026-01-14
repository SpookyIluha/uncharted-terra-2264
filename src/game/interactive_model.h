#ifndef INTERACTIVE_MODEL_H
#define INTERACTIVE_MODEL_H

#include "entity.h"
#include "engine_t3dm_wrapper.h"

#define INTERACTIVE_MODEL_TYPE_NAME "InteractiveModel"

#define INTERACTIVE_MODEL_ITEMINSERTED_BIT (1<<3)

class InteractiveModel : public Entity {
private:
    std::string model_path;
    std::string inserted_model_path;
    std::string on_use_command;
    std::string on_retrieve_command;
    std::string required_item;
    std::string wrong_item_subtitle;
    std::string on_inserted_subtitle;
    bool retrievable; // If true, item is kept. If false, item is consumed.
    float range;

    T3DMWModel model;
    T3DMWModel inserted_model;

    bool iteminserted;

public:
    InteractiveModel(const std::string& name, int id);
    virtual ~InteractiveModel() = default;

    void load_from_ini(const tortellini::ini::section& section) override;
    void load_from_eeprom(uint16_t data) override;
    uint16_t save_to_eeprom() const override;
    void init() override;
    void update() override;
    void draw() override;

    static Entity* create(const std::string& name, int id);
};

#endif
