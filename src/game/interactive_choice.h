 #ifndef INTERACTIVE_CHOICE_H
 #define INTERACTIVE_CHOICE_H
 
 #include "entity.h"
 
 #define INTERACTIVE_CHOICE_TYPE_NAME "InteractiveChoice"
 #define INTERACTIVE_CHOICE_USED_BIT (1<<3)

 class InteractiveChoice : public Entity {
 private:
     std::string title;
     std::string choice1_text;
     std::string choice2_text;
     std::string choice1_command;
     std::string choice2_command;
     bool forced_choice;
     float range;
     bool used;
 public:
     InteractiveChoice(const std::string& name, int id);
     virtual ~InteractiveChoice() = default;
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
 
 #endif
