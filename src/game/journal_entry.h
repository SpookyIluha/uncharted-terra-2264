#ifndef JOURNAL_ENTRY_H
#define JOURNAL_ENTRY_H

#include "entity.h"
#include "../engine_t3dm_wrapper.h"

#define JOURNAL_ENTRY_TYPE_NAME ("JournalEntry")

// Example entity class
class JournalEntry : public Entity {
private:
    int entry_id;
    bool pickedup;
    T3DMWModel journalmodel;

public:
    JournalEntry(const std::string& name, int id);
    virtual ~JournalEntry() = default;

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

