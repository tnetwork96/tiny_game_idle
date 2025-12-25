# Context
I am developing a retro arcade-style UI for ESP32 using Arduino framework, Adafruit_GFX, and Adafruit_ST7789 libraries. The display is 320x240 (Landscape).

# Task
Create a new C++ class for a screen named: **[TÊN MÀN HÌNH MỚI, ví dụ: InventoryScreen]**.

# Design System (Deep Space Arcade Theme)
Please strictly follow this visual style to match the existing BuddyListScreen:

1. **Colors (RGB565 Hex):**
   - Background: `0x0042` (Deep Midnight Blue)
   - Header Background: `0x08A5`
   - List Item Highlight: `0x1148` (Electric Blue tint)
   - Accent/Borders: `0x07FF` (Cyan/Neon Blue)
   - Text: `0xFFFF` (White) for main text, `0x07FF` (Cyan) for UI elements.
   - Positive/Online: `0x07F3` (Mint Green)
   - Negative/Offline: `0xF986` (Bright Red)

2. **Layout Specs:**
   - Screen Size: 320x240.
   - Header Height: 30px.
   - Row Height: 24px (for lists).
   - Font: Standard GFX font (size 1 or 2).

3. **UX/Interaction Pattern:**
   - Navigation: Physical buttons (Up, Down, Select).
   - Selection: When an item is selected, change its background color to `0x1148` and draw a thin 2px Cyan vertical bar on the left edge.
   - Scrollbar: If the content overflows, draw a custom scrollbar on the right (4px width).

# Code Structure Requirements
1. **Class Structure:**
   - Create `.h` and `.cpp` files.
   - The class must maintain a pointer to `Adafruit_ST7789`.
   - Implement `draw()` for full screen rendering.
   - Implement navigation methods: `handleUp()`, `handleDown()`, `handleSelect()`.

2. **Performance Optimization:**
   - Do NOT use `fillScreen()` inside navigation methods.
   - Implement a "Partial Redraw" strategy: When moving selection, only redraw the 'previous' row (to clear highlight) and the 'new' row (to add highlight).
   - Use a `scrollOffset` variable to handle list scrolling efficiently.

# Specific Functionality for this Screen
[MÔ TẢ CHỨC NĂNG MÀN HÌNH MỚI TẠI ĐÂY]
- Ví dụ: Màn hình hiển thị danh sách vật phẩm (Item, Quantity).
- Ví dụ: Có nút "Use" và "Drop" khi bấm Select vào vật phẩm.
- Ví dụ: Dữ liệu đầu vào là một struct ItemEntry.

Please generate the full code for .h and .cpp files.