#include "storage.h"
#include "config.h"
#include "state.h"
#include <Preferences.h>

// Fixed-size struct for flash storage (no String types)
struct ReminderStorage {
  int id;
  char message[64];
  time_t when;
  int limitMinutes;
  bool completed;
  uint16_t color;
  bool triggered;
  time_t nextReviewTime;
  int reviewCount;
};

static Preferences prefs;

void initStorage() {
  prefs.begin("reminders", false);  // false = read/write mode
  loadReminders();
  loadTodos();
  Serial.println("Storage initialized");
}

void saveReminders() {
  ReminderStorage storage[MAX_REMINDERS];

  // Convert from Reminder to ReminderStorage
  for (int i = 0; i < MAX_REMINDERS; i++) {
    storage[i].id = reminders[i].id;
    strncpy(storage[i].message, reminders[i].message.c_str(), 63);
    storage[i].message[63] = '\0';  // Ensure null termination
    storage[i].when = reminders[i].when;
    storage[i].limitMinutes = reminders[i].limitMinutes;
    storage[i].completed = reminders[i].completed;
    storage[i].color = reminders[i].color;
    storage[i].triggered = reminders[i].triggered;
    storage[i].nextReviewTime = reminders[i].nextReviewTime;
    storage[i].reviewCount = reminders[i].reviewCount;
  }

  // Save as bytes
  prefs.putBytes("data", storage, sizeof(storage));
  prefs.putInt("nextId", nextReminderId);

  Serial.println("Reminders saved to flash");
}

void loadReminders() {
  ReminderStorage storage[MAX_REMINDERS];

  // Check if data exists
  size_t len = prefs.getBytesLength("data");
  if (len == 0) {
    Serial.println("No stored reminders found");
    return;
  }

  // Load bytes
  prefs.getBytes("data", storage, sizeof(storage));
  nextReminderId = prefs.getInt("nextId", 1);

  // Convert from ReminderStorage to Reminder
  int loadedCount = 0;
  for (int i = 0; i < MAX_REMINDERS; i++) {
    // Basic sanitation: check for negative IDs or garbage data
    if (storage[i].id < 0 || storage[i].id > 100000) {
      reminders[i] = Reminder();
      continue;
    }

    reminders[i].id = storage[i].id;
    reminders[i].message = String(storage[i].message);
    reminders[i].when = storage[i].when;
    reminders[i].limitMinutes = storage[i].limitMinutes;
    reminders[i].completed = storage[i].completed;
    reminders[i].color = storage[i].color;
    reminders[i].triggered = storage[i].triggered;
    reminders[i].nextReviewTime = storage[i].nextReviewTime;
    reminders[i].reviewCount = storage[i].reviewCount;

    if (reminders[i].id != 0) loadedCount++;
  }

  Serial.printf("Loaded %d reminders from flash, nextId=%d\n", loadedCount, nextReminderId);
}

void clearStoredReminders() {
  prefs.remove("data");
  prefs.remove("nextId");
  Serial.println("Stored reminders cleared");
}

// ==================== Todo Storage ====================

// Fixed-size struct for todo flash storage
struct TodoStorage {
  char text[128];
  bool completed;
};

static Preferences todoPrefs;
static bool todoPrefsInitialized = false;

static void ensureTodoPrefs() {
  if (!todoPrefsInitialized) {
    todoPrefs.begin("todos", false);
    todoPrefsInitialized = true;
  }
}

void saveTodos() {
  ensureTodoPrefs();
  
  TodoStorage storage[MAX_TODO_ITEMS];
  
  // Convert from TodoItem to TodoStorage
  for (int i = 0; i < MAX_TODO_ITEMS; i++) {
    strncpy(storage[i].text, todoItems[i].text.c_str(), 127);
    storage[i].text[127] = '\0';
    storage[i].completed = todoItems[i].completed;
  }
  
  todoPrefs.putBytes("data", storage, sizeof(storage));
  todoPrefs.putInt("count", todoItemCount);
  
  Serial.printf("Saved %d todos to flash\n", todoItemCount);
}

void loadTodos() {
  ensureTodoPrefs();
  
  TodoStorage storage[MAX_TODO_ITEMS];
  
  size_t len = todoPrefs.getBytesLength("data");
  if (len == 0) {
    Serial.println("No stored todos found");
    return;
  }
  
  todoPrefs.getBytes("data", storage, sizeof(storage));
  todoItemCount = todoPrefs.getInt("count", 0);
  
  // Convert from TodoStorage to TodoItem
  for (int i = 0; i < MAX_TODO_ITEMS; i++) {
    todoItems[i].text = String(storage[i].text);
    todoItems[i].completed = storage[i].completed;
  }
  
  Serial.printf("Loaded %d todos from flash\n", todoItemCount);
}

void clearStoredTodos() {
  ensureTodoPrefs();
  todoPrefs.remove("data");
  todoPrefs.remove("count");
  todoItemCount = 0;
  for (int i = 0; i < MAX_TODO_ITEMS; i++) {
    todoItems[i] = TodoItem();
  }
  Serial.println("Stored todos cleared");
}
