#include "confirmation_dialog.h"

// Deep Space Arcade Theme (matching Login Screen)
#define WIN_BG_DARK   0x0042  // Deep Midnight Blue #020817
#define WIN_HEADER    0x08A5  // Header Blue #0F172A
#define WIN_ACCENT    0x07FF  // Cyan accent
#define WIN_TEXT      0xFFFF  // White text

ConfirmationDialog::ConfirmationDialog(Adafruit_ST7789* tft) {
    this->tft = tft;
    this->visible = false;
    this->selectedButton = 1;  // Default to Cancel (NO) for safety
    this->onConfirmCallback = nullptr;
    this->onCancelCallback = nullptr;
    this->borderColor = WIN_ACCENT;  // Default Cyan accent
    this->buttonPositionsCached = false;
}

void ConfirmationDialog::show(const String& message, 
                              const String& confirmLabel,
                              const String& cancelLabel,
                              ConfirmCallback onConfirm,
                              CancelCallback onCancel,
                              uint16_t borderColor) {
    this->message = message;
    this->confirmLabel = confirmLabel;
    this->cancelLabel = cancelLabel;
    this->onConfirmCallback = onConfirm;
    this->onCancelCallback = onCancel;
    this->borderColor = borderColor;
    this->selectedButton = 1;  // Default to Cancel (NO) for safety
    this->visible = true;
    this->buttonPositionsCached = false;  // Reset cache - will be set on first draw()
}

void ConfirmationDialog::hide() {
    this->visible = false;
    this->message = "";
    this->onConfirmCallback = nullptr;
    this->onCancelCallback = nullptr;
}

void ConfirmationDialog::draw() {
    if (!visible || tft == nullptr) return;
    
    const uint16_t screenWidth = 320;
    const uint16_t screenHeight = 240;
    const uint16_t headerHeight = 30;
    
    // Draw overlay (dimmed background)
    drawOverlay();
    
    // Draw full-screen background (Deep Space theme)
    tft->fillScreen(WIN_BG_DARK);
    
    // Draw header bar matching Login Screen style
    tft->fillRect(0, 0, screenWidth, headerHeight, WIN_HEADER);
    tft->drawFastHLine(0, headerHeight - 1, screenWidth, WIN_ACCENT);
    
    // Title in header
    tft->setTextSize(2);
    tft->setTextColor(WIN_TEXT, WIN_HEADER);
    tft->setCursor(10, 8);
    tft->print("CONFIRM");
    
    // Draw buttons at Y = 130 (headerHeight + 100)
    uint16_t buttonY = headerHeight + 100;
    uint16_t buttonWidth = (POPUP_WIDTH - (POPUP_PADDING * 2) - BUTTON_SPACING) / 2;
    uint16_t popupWidth = POPUP_WIDTH;
    uint16_t popupX = (screenWidth - popupWidth) / 2;
    uint16_t buttonStartX = popupX + POPUP_PADDING;
    
    // Calculate popup dimensions (centered below header, ending before buttons)
    // Buttons are at Y=130, so popup should end around Y=120
    uint16_t maxPopupHeight = buttonY - headerHeight - 30;  // Leave space above buttons
    uint16_t popupHeight = calculatePopupHeight();
    if (popupHeight > maxPopupHeight) {
        popupHeight = maxPopupHeight;
    }
    uint16_t popupY = headerHeight + 20;  // Below header with spacing
    
    // Draw popup box background (Deep Midnight Blue)
    tft->fillRoundRect(popupX, popupY, popupWidth, popupHeight, 8, BOX_BG_COLOR);
    
    // Draw border (Cyan accent)
    tft->drawRoundRect(popupX, popupY, popupWidth, popupHeight, 8, WIN_ACCENT);
    tft->drawRoundRect(popupX + 1, popupY + 1, popupWidth - 2, popupHeight - 2, 7, WIN_ACCENT);
    
    // Draw message (with text wrapping)
    uint16_t messageX = popupX + POPUP_PADDING;
    uint16_t messageY = popupY + POPUP_PADDING;
    uint16_t messageWidth = popupWidth - (POPUP_PADDING * 2);
    drawMessage(messageX, messageY, messageWidth);
    
    // Cache button positions for partial redraw
    confirmButtonX = buttonStartX;
    confirmButtonY = buttonY;
    confirmButtonWidth = buttonWidth;
    cancelButtonX = buttonStartX + buttonWidth + BUTTON_SPACING;
    cancelButtonY = buttonY;
    cancelButtonWidth = buttonWidth;
    buttonPositionsCached = true;
    
    // Confirm button (left)
    bool confirmSelected = (selectedButton == 0);
    drawButton(confirmButtonX, confirmButtonY, confirmButtonWidth, confirmLabel, CONFIRM_COLOR, confirmSelected);
    
    // Cancel button (right)
    bool cancelSelected = (selectedButton == 1);
    drawButton(cancelButtonX, cancelButtonY, cancelButtonWidth, cancelLabel, CANCEL_COLOR, cancelSelected);
}

void ConfirmationDialog::drawOverlay() {
    const uint16_t screenWidth = 320;
    const uint16_t screenHeight = 240;
    
    // Draw semi-transparent overlay (dimming effect)
    tft->fillRect(0, 0, screenWidth, screenHeight, OVERLAY_COLOR);
}

void ConfirmationDialog::drawMessage(uint16_t x, uint16_t y, uint16_t width) {
    tft->setTextSize(1);
    tft->setTextColor(TEXT_COLOR, BOX_BG_COLOR);
    
    // Simple text wrapping (split by spaces and fit within width)
    const uint16_t charWidth = 6;  // Text size 1 ~6px per char
    const uint16_t lineHeight = 10;  // Text size 1 ~8px + 2px spacing
    uint16_t maxCharsPerLine = width / charWidth;
    
    String remainingText = message;
    uint16_t currentY = y;
    uint16_t maxLines = 4;  // Max 4 lines to fit in popup
    int lineCount = 0;
    
    while (remainingText.length() > 0 && lineCount < maxLines) {
        // Find the longest substring that fits
        String line = "";
        if (remainingText.length() <= maxCharsPerLine) {
            line = remainingText;
            remainingText = "";
        } else {
            // Try to break at space
            int breakPos = maxCharsPerLine;
            for (int i = maxCharsPerLine; i > 0; i--) {
                if (remainingText.charAt(i) == ' ') {
                    breakPos = i;
                    break;
                }
            }
            line = remainingText.substring(0, breakPos);
            remainingText = remainingText.substring(breakPos + 1);
        }
        
        // Center-align text
        uint16_t textX = x + (width - line.length() * charWidth) / 2;
        tft->setCursor(textX, currentY);
        tft->print(line);
        
        currentY += lineHeight;
        lineCount++;
    }
}

void ConfirmationDialog::drawButton(uint16_t x, uint16_t y, uint16_t width, const String& label, uint16_t color, bool selected) {
    // Deep Space theme button styling:
    // Selected: Accent Background (Cyan) + Dark Text (Header Blue)
    // Unselected: Dark Background + Accent Border + Accent Text
    uint16_t bgColor = selected ? WIN_ACCENT : WIN_BG_DARK;
    uint16_t textColor = selected ? WIN_HEADER : WIN_ACCENT;
    uint16_t borderColor = WIN_ACCENT;
    
    // Draw button background
    tft->fillRoundRect(x, y, width, BUTTON_HEIGHT, 4, bgColor);
    
    // Draw border (always Cyan accent)
    tft->drawRoundRect(x, y, width, BUTTON_HEIGHT, 4, borderColor);
    
    // Draw label text (center-aligned)
    tft->setTextColor(textColor, bgColor);
    tft->setTextSize(1);
    uint16_t textX = x + (width - label.length() * 6) / 2;  // ~6px per char
    uint16_t textY = y + (BUTTON_HEIGHT - 8) / 2;  // Center vertically
    tft->setCursor(textX, textY);
    tft->print(label);
}

uint16_t ConfirmationDialog::calculatePopupHeight() const {
    // Calculate height based on message length
    const uint16_t charWidth = 6;
    const uint16_t lineHeight = 10;
    const uint16_t messageWidth = POPUP_WIDTH - (POPUP_PADDING * 2);
    uint16_t maxCharsPerLine = messageWidth / charWidth;
    
    // Estimate lines needed
    int estimatedLines = (message.length() + maxCharsPerLine - 1) / maxCharsPerLine;
    if (estimatedLines > 4) estimatedLines = 4;  // Cap at 4 lines
    
    // Height = padding + message lines + button area
    uint16_t height = POPUP_PADDING * 2 + (estimatedLines * lineHeight) + BUTTON_HEIGHT + POPUP_PADDING;
    
    // Ensure minimum height
    if (height < POPUP_MIN_HEIGHT) {
        height = POPUP_MIN_HEIGHT;
    }
    
    return height;
}

void ConfirmationDialog::drawButtonSelection() {
    // Partial redraw: only update button selection states
    // This avoids flickering by not redrawing the entire dialog
    if (!visible || !buttonPositionsCached || tft == nullptr) return;
    
    // Redraw both buttons with their current selection states
    bool confirmSelected = (selectedButton == 0);
    bool cancelSelected = (selectedButton == 1);
    
    drawButton(confirmButtonX, confirmButtonY, confirmButtonWidth, confirmLabel, CONFIRM_COLOR, confirmSelected);
    drawButton(cancelButtonX, cancelButtonY, cancelButtonWidth, cancelLabel, CANCEL_COLOR, cancelSelected);
}

void ConfirmationDialog::handleLeft() {
    if (!visible) return;
    if (selectedButton == 1) {
        // Move from Cancel to Confirm
        selectedButton = 0;
        // Use partial redraw instead of full draw to prevent flicker
        drawButtonSelection();
    }
}

void ConfirmationDialog::handleRight() {
    if (!visible) return;
    if (selectedButton == 0) {
        // Move from Confirm to Cancel
        selectedButton = 1;
        // Use partial redraw instead of full draw to prevent flicker
        drawButtonSelection();
    }
}

void ConfirmationDialog::handleSelect() {
    if (!visible) return;
    
    if (selectedButton == 0) {
        // Confirm selected
        if (onConfirmCallback != nullptr) {
            onConfirmCallback();
        }
    } else {
        // Cancel selected
        if (onCancelCallback != nullptr) {
            onCancelCallback();
        }
    }
    
    hide();
}

void ConfirmationDialog::handleCancel() {
    if (!visible) return;
    
    // Trigger cancel callback
    if (onCancelCallback != nullptr) {
        onCancelCallback();
    }
    
    hide();
}

void ConfirmationDialog::wrapText(const String& text, uint16_t maxWidth, String* lines, int& lineCount) const {
    // Helper for text wrapping (if needed in future)
    lineCount = 0;
}

