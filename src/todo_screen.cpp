#include "todo_screen.h"
#include "state.h"
#include "config.h"
#include "screen.h"
#include "fonts/MDIOTrial_Regular9pt7b.h"
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

// Sprite for flicker-free rendering
static TFT_eSprite todoSprite = TFT_eSprite(&tft);
static bool todoSpriteCreated = false;
static bool todoSpriteAttempted = false;

// Track first draw for screen reset
static bool todoFirstDraw = true;

// ==================== Helper: Draw Checkmark ====================
static void drawCheckmark(TFT_eSPI& canvas, int x, int y, int size) {
  // Draw green checkmark (tick) inside the bullet square - no fill
  int cx = x + size / 2;
  int cy = y + size / 2;
  
  // Checkmark points (V shape)
  int x1 = x + 2;
  int y1 = cy;
  int x2 = cx - 1;
  int y2 = y + size - 3;
  int x3 = x + size - 2;
  int y3 = y + 2;
  
  // Green tick lines
  canvas.drawLine(x1, y1, x2, y2, COLOR_TODO_CHECK);
  canvas.drawLine(x2, y2, x3, y3, COLOR_TODO_CHECK);
  // Thicker line
  canvas.drawLine(x1, y1 + 1, x2, y2 + 1, COLOR_TODO_CHECK);
  canvas.drawLine(x2, y2 + 1, x3, y3 + 1, COLOR_TODO_CHECK);
}

// ==================== Helper: Word Wrap ====================
// Returns number of visual lines used
static int drawWrappedText(TFT_eSPI& canvas, const String& text, int x, int y, int maxWidth, int lineHeight, int maxLines, int& linesUsed) {
  int currentY = y;
  linesUsed = 0;
  
  if (text.length() == 0) {
    linesUsed = 1;
    return 1;
  }
  
  int textLen = text.length();
  int startIdx = 0;
  
  while (startIdx < textLen && linesUsed < maxLines) {
    // Find how many chars fit on this line
    int endIdx = startIdx;
    int lastSpace = -1;
    
    while (endIdx < textLen) {
      String testStr = text.substring(startIdx, endIdx + 1);
      int testWidth = canvas.textWidth(testStr);
      
      if (testWidth > maxWidth) {
        // Too wide - break at last space or here
        if (lastSpace > startIdx) {
          endIdx = lastSpace;
        }
        break;
      }
      
      if (text.charAt(endIdx) == ' ') {
        lastSpace = endIdx;
      }
      endIdx++;
    }
    
    if (endIdx == startIdx) {
      endIdx = startIdx + 1;  // Force at least one char
    }
    
    // Draw this line
    String line = text.substring(startIdx, endIdx);
    line.trim();
    canvas.drawString(line, x, currentY);
    
    linesUsed++;
    currentY += lineHeight;
    
    // Skip spaces and move to next line
    startIdx = endIdx;
    while (startIdx < textLen && text.charAt(startIdx) == ' ') {
      startIdx++;
    }
  }
  
  return linesUsed;
}

// ==================== Draw Content ====================
void drawTodoContent() {
  static const int todoW = 320;
  static const int todoH = 195;  // 240 - 45 = 195
  const int zoneY = 45;

  // Create sprite for flicker-free rendering
  if (!todoSpriteAttempted) {
    todoSpriteAttempted = true;
    todoSprite.setColorDepth(8);  // 8-bit to save RAM
    void* ptr = todoSprite.createSprite(todoW, todoH);
    if (ptr == nullptr) {
      Serial.println("CRITICAL: Failed to create todo sprite");
      todoSpriteCreated = false;
    } else {
      Serial.printf("SUCCESS: Todo sprite (8-bit) created\n");
      todoSpriteCreated = true;
    }
  }

  // Define drawing surface
  TFT_eSPI& canvas = todoSpriteCreated ? (TFT_eSPI&)todoSprite : tft;
  int yOffset = todoSpriteCreated ? 0 : zoneY;

  // Clear background
  if (todoSpriteCreated) {
    todoSprite.fillSprite(COLOR_BACKGROUND);
  } else {
    tft.fillRect(0, zoneY, todoW, todoH, COLOR_BACKGROUND);
  }

  canvas.setFreeFont(&MDIOTrial_Regular9pt7b);
  canvas.setTextSize(1);
  canvas.setTextColor(COLOR_TODO_TEXT);

  // Track visual lines used
  int currentLine = 0;
  int itemIdx = 0;

  // Draw todo items
  while (itemIdx < todoItemCount && currentLine < TODO_MAX_LINES) {
    TodoItem& item = todoItems[itemIdx];
    
    int bulletY = yOffset + (currentLine * TODO_LINE_HEIGHT) + 8;
    int textY = yOffset + (currentLine * TODO_LINE_HEIGHT) + 4;
    
    // Draw lavender bullet square (1px outline, not filled)
    canvas.drawRect(TODO_BULLET_X, bulletY, TODO_BULLET_SIZE, TODO_BULLET_SIZE, COLOR_TODO_BULLET);
    
    // Draw checkmark if completed
    if (item.completed) {
      drawCheckmark(canvas, TODO_BULLET_X, bulletY, TODO_BULLET_SIZE);
    }
    
    // Draw text with word wrap
    int linesUsed = 0;
    int remainingLines = TODO_MAX_LINES - currentLine;
    drawWrappedText(canvas, item.text, TODO_TEXT_X, textY, TODO_TEXT_WIDTH, TODO_LINE_HEIGHT, remainingLines, linesUsed);
    
    currentLine += linesUsed;
    itemIdx++;
  }

  // If no todos, show message
  if (todoItemCount == 0) {
    canvas.setTextColor(TFT_DARKGREY);
    canvas.drawString("No tasks. Use /todo API to add.", 20, yOffset + 80);
  }

#if DEBUG_NO_BG_CONTENT_ZONE
  // Draw debug border for content zone
  if (todoSpriteCreated) {
    todoSprite.drawRect(0, 0, todoW, todoH, TFT_WHITE);
  } else {
    tft.drawRect(0, zoneY, todoW, todoH, TFT_WHITE);
  }
#endif

  // Push to screen
  if (todoSpriteCreated) {
    todoSprite.pushSprite(0, zoneY);
  }

  todoFirstDraw = false;
}

// Reset for fresh draw when entering screen
void resetTodoScreen() {
  todoFirstDraw = true;
}
