#ifndef AUTO_NAVIGATOR_H
#define AUTO_NAVIGATOR_H

#include <Arduino.h>
#include <SPIFFS.h>

// Auto Navigator - Reads navigation commands from file or serial and executes them
class AutoNavigator {
public:
    enum CommandType {
        CMD_UP,
        CMD_DOWN,
        CMD_LEFT,
        CMD_RIGHT,
        CMD_SELECT,
        CMD_EXIT,
        CMD_DELAY,
        CMD_WAIT,
        CMD_UNKNOWN
    };

    AutoNavigator();
    ~AutoNavigator() = default;

    // Load commands from file
    bool loadFromFile(const String& filename);

    // Load commands from string (comma or newline separated)
    bool loadFromString(const String& commands);

    // Execute next command (returns true if command executed, false if queue empty)
    bool executeNext();

    // Execute all commands in queue
    void executeAll();

    // Check if there are more commands to execute
    bool hasMoreCommands() const { return currentIndex < commandQueue.length(); }

    // Clear command queue
    void clear();

    // Get current command count
    int getCommandCount() const { return commandQueue.length(); }

    // Set callback function for executing commands
    typedef void (*CommandCallback)(const String& command);
    void setCommandCallback(CommandCallback callback) { commandCallback = callback; }

    // Set delay between commands (in milliseconds)
    void setCommandDelay(unsigned long delayMs) { commandDelay = delayMs; }

    // Enable/disable auto execution
    void setEnabled(bool enabled) { isEnabled = enabled; }
    bool getEnabled() const { return isEnabled; }

    // Process serial input for auto navigation
    void processSerialInput();

private:
    String commandQueue;
    int currentIndex;
    unsigned long lastCommandTime;
    unsigned long commandDelay;
    bool isEnabled;
    CommandCallback commandCallback;

    // Parse command string to CommandType
    CommandType parseCommand(const String& cmd);

    // Convert CommandType to command string
    String commandToString(CommandType cmd);

    // Execute a single command
    void executeCommand(CommandType cmd);
};

#endif

