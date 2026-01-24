#ifndef INTERACTIVE_KEYPAD_H
#define INTERACTIVE_KEYPAD_H
#
#include "entity.h"
#
#define INTERACTIVE_KEYPAD_TYPE_NAME "InteractiveKeypad"
#define INTERACTIVE_KEYPAD_UNLOCKED_BIT (1<<3)
#
class InteractiveKeypad : public Entity {
private:
    int digit_count;
    std::string correct_password;
    std::string on_success_command;
    std::string success_subtitle;
    float range;
    bool unlocked;
public:
    InteractiveKeypad(const std::string& name, int id);
    virtual ~InteractiveKeypad() = default;
    void load_from_ini(const tortellini::ini::section& section) override;
    void load_from_eeprom(uint16_t data) override;
    uint16_t save_to_eeprom() const override;
    void init() override;
    void update() override;
    void draw() override;
    static Entity* create(const std::string& name, int id);
private:
    void open_menu();
};
#
#endif
