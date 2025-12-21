#ifndef CONFIRMATION_DIALOG_H
#define CONFIRMATION_DIALOG_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// Callback function types
typedef void (*ConfirmCallback)();
typedef void (*CancelCallback)();

class ConfirmationDialog {
public:
    ConfirmationDialog(Adafruit_ST7789* tft);
    
    // Show dialog with configuration
    void show(const String& message, 
              const String& confirmLabel = "YES",
              const String& cancelLabel = "NO",
              ConfirmCallback onConfirm = nullptr,
              CancelCallback onCancel = nullptr,
              uint16_t borderColor = 0xF81F);  // Default Neon Pink/Red for alerts
    
    // Hide dialog
    void hide();
    
    // Check if dialog is visible
    bool isVisible() const { return visible; }
    
    // Draw the dialog
    void draw();
    
    // Partial redraw: only update button selection states (for flicker-free navigation)
    void drawButtonSelection();
    
    // Navigation
    void handleLeft();
    void handleRight();
    void handleSelect();
    void handleCancel();  // For BACK/ESC
    
    // Get current selection (0 = Confirm, 1 = Cancel)
    uint8_t getSelectedButton() const { return selectedButton; }

private:
    Adafruit_ST7789* tft;
    
    bool visible;
    String message;
    String confirmLabel;
    String cancelLabel;
    ConfirmCallback onConfirmCallback;
    CancelCallback onCancelCallback;
    uint16_t borderColor;
    uint8_t selectedButton;  // 0 = Confirm, 1 = Cancel (default)
    
    // Cached button positions for partial redraw (set during draw())
    uint16_t confirmButtonX;
    uint16_t confirmButtonY;
    uint16_t confirmButtonWidth;
    uint16_t cancelButtonX;
    uint16_t cancelButtonY;
    uint16_t cancelButtonWidth;
    bool buttonPositionsCached;  // Track if positions are valid
    
    // Colors (Midnight Blue theme)
    static const uint16_t OVERLAY_COLOR = 0x0004;      // Very dark blue (semi-transparent effect)
    static const uint16_t BOX_BG_COLOR = 0x0F17;      // Deep Navy (#0F172A)
    static const uint16_t TEXT_COLOR = 0xFFFF;        // White
    static const uint16_t CONFIRM_COLOR = 0xF800;     // Neon Red (for destructive)
    static const uint16_t CANCEL_COLOR = 0x07FF;      // Neon Cyan
    static const uint16_t SELECTED_BG = 0xFE20;       // Yellow-Orange (selected background)
    
    // Layout constants
    static const uint16_t POPUP_WIDTH = 200;
    static const uint16_t POPUP_MIN_HEIGHT = 80;
    static const uint16_t POPUP_PADDING = 10;
    static const uint16_t BUTTON_HEIGHT = 30;
    static const uint16_t BUTTON_SPACING = 10;
    
    // Helper methods
    uint16_t calculatePopupHeight() const;
    void drawOverlay();
    void drawMessage(uint16_t x, uint16_t y, uint16_t width);
    void drawButton(uint16_t x, uint16_t y, uint16_t width, const String& label, uint16_t color, bool selected);
    void wrapText(const String& text, uint16_t maxWidth, String* lines, int& lineCount) const;
};

#endif

