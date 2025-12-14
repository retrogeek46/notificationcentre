#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>

// Initialize storage (call once in setup)
void initStorage();

// Save all reminders to flash
void saveReminders();

// Load reminders from flash (called by initStorage)
void loadReminders();

// Clear all stored reminders
void clearStoredReminders();

#endif
