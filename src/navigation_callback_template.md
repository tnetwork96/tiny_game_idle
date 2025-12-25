# Navigation Callback Template

## Purpose
This template provides a guide for implementing event-driven navigation between screens using callback mechanisms, avoiding polling-based approaches that are inefficient and violate clean code principles.

## Problem Pattern to Avoid
❌ **DON'T**: Use polling flags that need to be checked in the main loop
```cpp
// BAD: Polling approach
class SomeScreen {
    bool shouldExit;  // Flag that needs polling
    bool isDismissed() const { return shouldExit; }
};

// In main loop - BAD
if (someScreen->isDismissed()) {
    // Navigate away
}
```

## Solution Pattern
✅ **DO**: Use callback mechanism for immediate, event-driven navigation

## Implementation Steps

### Step 1: Define Callback Type in Header File

```cpp
// In your_screen.h (before class definition)
typedef void (*ExitCallback)();
```

### Step 2: Add Private Member and Public Setter

```cpp
// In your_screen.h (private section)
class YourScreen {
private:
    // ... other members ...
    ExitCallback onExitCallback;  // Private callback member
    
public:
    // ... other methods ...
    void setOnExitCallback(ExitCallback callback);  // Public setter
};
```

### Step 3: Initialize and Implement in .cpp File

```cpp
// In your_screen.cpp constructor
YourScreen::YourScreen(...) {
    // ... other initialization ...
    this->onExitCallback = nullptr;  // Initialize to null
}

// Implement setter
void YourScreen::setOnExitCallback(ExitCallback callback) {
    onExitCallback = callback;
}

// Use callback when action triggers exit
void YourScreen::onSomeAction() {
    // Perform action logic
    Serial.println("Action completed");
    
    // Invoke callback immediately - no intermediate steps
    if (onExitCallback != nullptr) {
        onExitCallback();
    }
    // DON'T add messages, DON'T redraw screen - let callback handle navigation
}
```

### Step 4: Skip Redraw When Exiting

```cpp
// In handleSelect() or similar method
void YourScreen::handleSelect() {
    if (dialog != nullptr && dialog->isVisible()) {
        dialog->handleSelect();
        
        // If action triggers exit, skip redraw
        if (pendingAction == EXIT_ACTION && !dialog->isVisible()) {
            pendingAction = 0;
            // Return immediately - let callback handle navigation
            return;  // DON'T call draw() here
        }
        
        // Normal flow for other actions
        needsRedraw = true;
        draw();
        return;
    }
}
```

### Step 5: Create Callback Function in main.cpp

```cpp
// In main.cpp (before setup())
void onYourScreenExit() {
    Serial.println("Main: Screen exit callback triggered");
    
    // Update game state
    inGame = false;
    currentGame = 0;  // or appropriate screen ID
    
    // Clear screen
    tft.fillScreen(ST77XX_BLACK);
    
    // Navigate to target screen
    if (targetScreen != nullptr) {
        targetScreen->draw();
    }
}
```

### Step 6: Register Callback in setup()

```cpp
// In setup()
void setup() {
    // ... other initialization ...
    
    // Create screen instance
    yourScreen = new YourScreen(&tft, ...);
    
    // Register exit callback
    yourScreen->setOnExitCallback(onYourScreenExit);
    
    // ... rest of setup ...
}
```

## Key Principles

### ✅ DO:
1. **Define callback type before class** - Makes it available throughout the file
2. **Initialize callback to nullptr** - Safe default state
3. **Check for nullptr before invoking** - Prevents crashes
4. **Invoke callback immediately** - No intermediate steps (messages, redraws)
5. **Skip redraw when exiting** - Let callback handle navigation
6. **Register in setup()** - Ensures callback is available when needed

### ❌ DON'T:
1. **Don't add messages before exit** - User won't see them anyway
2. **Don't redraw screen before exit** - Creates unnecessary intermediate step
3. **Don't use polling flags** - Inefficient and violates clean code
4. **Don't check flags in loop()** - Use callbacks instead
5. **Don't delay callback invocation** - Should be immediate

## Example: Complete Implementation

### Header File (your_screen.h)
```cpp
#ifndef YOUR_SCREEN_H
#define YOUR_SCREEN_H

#include <Adafruit_ST7789.h>

// Callback type for exiting screen
typedef void (*ExitCallback)();

class YourScreen {
private:
    Adafruit_ST7789* tft;
    ExitCallback onExitCallback;  // Private callback
    
public:
    YourScreen(Adafruit_ST7789* tft);
    void setOnExitCallback(ExitCallback callback);
    void onActionConfirm();
    void handleSelect();
};

#endif
```

### Implementation File (your_screen.cpp)
```cpp
#include "your_screen.h"

YourScreen::YourScreen(Adafruit_ST7789* tft) {
    this->tft = tft;
    this->onExitCallback = nullptr;  // Initialize
}

void YourScreen::setOnExitCallback(ExitCallback callback) {
    onExitCallback = callback;
}

void YourScreen::onActionConfirm() {
    Serial.println("Action confirmed");
    // Invoke callback immediately - no intermediate steps
    if (onExitCallback != nullptr) {
        onExitCallback();
    }
}

void YourScreen::handleSelect() {
    if (dialog != nullptr && dialog->isVisible()) {
        dialog->handleSelect();
        if (shouldExit && !dialog->isVisible()) {
            // Skip redraw - callback will handle navigation
            return;
        }
        // Normal flow
        draw();
    }
}
```

### Main File (main.cpp)
```cpp
// Callback function
void onYourScreenExit() {
    inGame = false;
    currentGame = 0;
    tft.fillScreen(ST77XX_BLACK);
    if (targetScreen != nullptr) {
        targetScreen->draw();
    }
}

void setup() {
    // ... initialization ...
    yourScreen = new YourScreen(&tft);
    yourScreen->setOnExitCallback(onYourScreenExit);
    // ... rest of setup ...
}

void loop() {
    // NO polling checks needed - callbacks handle everything
    // ... other loop logic ...
}
```

## Benefits

1. **Event-Driven**: Navigation happens immediately when event occurs
2. **Efficient**: No continuous polling in main loop
3. **Clean Code**: Separation of concerns - screen doesn't depend on main variables
4. **Maintainable**: Easy to understand and modify
5. **No Intermediate Steps**: Direct navigation without unnecessary redraws

## Common Mistakes to Avoid

1. ❌ Adding messages before exit (user won't see them)
2. ❌ Redrawing screen before exit (unnecessary intermediate step)
3. ❌ Forgetting to check `nullptr` before invoking callback
4. ❌ Not initializing callback in constructor
5. ❌ Using polling flags instead of callbacks
6. ❌ Checking flags in `loop()` instead of using callbacks

## Checklist

When implementing navigation callbacks, ensure:
- [ ] Callback type defined before class
- [ ] Private member variable added
- [ ] Public setter method added
- [ ] Callback initialized to `nullptr` in constructor
- [ ] Setter method implemented
- [ ] Callback invoked immediately when action triggers
- [ ] No intermediate steps (messages, redraws) before callback
- [ ] Redraw skipped when exiting
- [ ] Callback function created in main.cpp
- [ ] Callback registered in setup()
- [ ] No polling checks in loop()

