#ifndef SLIDING_DOOR_H
#define SLIDING_DOOR_H

#include "entity.h"
#include "engine_t3dm_wrapper.h"

#define SLIDING_DOOR_TYPE_NAME "SlidingDoor"

#define SLIDING_DOOR_OPENED_BIT (1<<3)

class SlidingDoor : public Entity {
private:
    std::string model_path;
    fm_vec3_t direction;
    int collision_associated_id;
    float move_distance;
    float speed;
    
    enum State {
        CLOSED,
        OPENING,
        OPEN,
        CLOSING
    };
    
    State state;
    float current_progress; // 0.0 to 1.0
    
    T3DMWModel model;
    fm_vec3_t initial_position;

public:
    SlidingDoor(const std::string& name, int id);
    virtual ~SlidingDoor() = default;

    void load_from_ini(const tortellini::ini::section& section) override;
    void load_from_eeprom(uint16_t data) override;
    uint16_t save_to_eeprom() const override;
    void init() override;
    void update() override;
    void draw() override;

    static Entity* create(const std::string& name, int id);
};

#endif
