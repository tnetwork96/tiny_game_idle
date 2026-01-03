#include "auto_navigator.h"

AutoNavigator::AutoNavigator() {
    currentIndex = 0;
    lastCommandTime = 0;
    commandDelay = 200;  // Default 200ms delay between commands
    isEnabled = false;
    commandCallback = nullptr;
}

bool AutoNavigator::loadFromFile(const String& filename) {
    // Initialize SPIFFS if not already initialized
    if (!SPIFFS.begin(true)) {
        Serial.print("AutoNavigator: SPIFFS initialization failed for file: ");
        Serial.println(filename);
        return false;
    }

    // Check if file exists
    if (!SPIFFS.exists(filename)) {
        Serial.print("AutoNavigator: File does not exist: ");
        Serial.println(filename);
        return false;
    }

    // Open file for reading
    File file = SPIFFS.open(filename, "r");
    if (!file) {
        Serial.print("AutoNavigator: Failed to open file: ");
        Serial.println(filename);
        return false;
    }

    // Read entire file content
    commandQueue = file.readString();
    file.close();

    // Clean up: remove carriage returns and normalize
    commandQueue.replace("\r", "");
    commandQueue.trim();

    currentIndex = 0;
    lastCommandTime = 0;

    Serial.print("AutoNavigator: Loaded ");
    Serial.print(commandQueue.length());
    Serial.print(" characters from file: ");
    Serial.println(filename);

    return true;
}

bool AutoNavigator::loadFromString(const String& commands) {
    commandQueue = commands;
    commandQueue.replace("\r", "");
    commandQueue.trim();
    currentIndex = 0;
    lastCommandTime = 0;

    Serial.print("AutoNavigator: Loaded commands from string (");
    Serial.print(commandQueue.length());
    Serial.println(" characters)");

    return true;
}

AutoNavigator::CommandType AutoNavigator::parseCommand(const String& cmd) {
    String lowerCmd = cmd;
    lowerCmd.toLowerCase();
    lowerCmd.trim();

    if (lowerCmd == "up" || lowerCmd == "u") {
        return CMD_UP;
    } else if (lowerCmd == "down" || lowerCmd == "d") {
        return CMD_DOWN;
    } else if (lowerCmd == "left" || lowerCmd == "l") {
        return CMD_LEFT;
    } else if (lowerCmd == "right" || lowerCmd == "r") {
        return CMD_RIGHT;
    } else if (lowerCmd == "select" || lowerCmd == "s" || lowerCmd == "enter" || lowerCmd == "e") {
        return CMD_SELECT;
    } else if (lowerCmd == "exit" || lowerCmd == "x" || lowerCmd == "back" || lowerCmd == "b") {
        return CMD_EXIT;
    } else if (lowerCmd.startsWith("delay:") || lowerCmd.startsWith("wait:")) {
        return CMD_DELAY;
    } else if (lowerCmd == "wait") {
        return CMD_WAIT;
    }

    return CMD_UNKNOWN;
}

String AutoNavigator::commandToString(CommandType cmd) {
    switch (cmd) {
        case CMD_UP: return "up";
        case CMD_DOWN: return "down";
        case CMD_LEFT: return "left";
        case CMD_RIGHT: return "right";
        case CMD_SELECT: return "select";
        case CMD_EXIT: return "exit";
        case CMD_DELAY: return "delay";
        case CMD_WAIT: return "wait";
        default: return "unknown";
    }
}

void AutoNavigator::executeCommand(CommandType cmd) {
    if (cmd == CMD_UNKNOWN) {
        return;
    }

    String cmdStr = commandToString(cmd);

    // Handle delay command
    if (cmd == CMD_DELAY) {
        // Extract delay value from command queue
        int colonIndex = commandQueue.indexOf(':', currentIndex);
        if (colonIndex != -1) {
            String delayStr = commandQueue.substring(colonIndex + 1);
            int nextNewline = delayStr.indexOf('\n');
            if (nextNewline != -1) {
                delayStr = delayStr.substring(0, nextNewline);
            }
            delayStr.trim();
            unsigned long delayMs = delayStr.toInt();
            if (delayMs > 0) {
                Serial.print("AutoNavigator: Delaying ");
                Serial.print(delayMs);
                Serial.println("ms");
                delay(delayMs);
            }
        }
        return;
    }

    // Handle wait command (wait for user input or condition)
    if (cmd == CMD_WAIT) {
        Serial.println("AutoNavigator: Waiting...");
        // Wait can be implemented to wait for specific conditions
        return;
    }

    // Execute navigation command via callback
    if (commandCallback != nullptr) {
        Serial.print("AutoNavigator: Executing command: ");
        Serial.println(cmdStr);
        commandCallback(cmdStr);
    } else {
        Serial.print("AutoNavigator: Warning - No callback set for command: ");
        Serial.println(cmdStr);
    }
}

bool AutoNavigator::executeNext() {
    if (!isEnabled) {
        return false;
    }

    if (currentIndex >= commandQueue.length()) {
        return false;  // No more commands
    }

    // Check if enough time has passed since last command
    unsigned long currentTime = millis();
    if (currentTime - lastCommandTime < commandDelay) {
        return false;  // Wait for delay
    }

    // Find next command (separated by newline, comma, or space)
    int nextIndex = commandQueue.length();
    int newlineIndex = commandQueue.indexOf('\n', currentIndex);
    int commaIndex = commandQueue.indexOf(',', currentIndex);
    int spaceIndex = commandQueue.indexOf(' ', currentIndex);

    // Find the earliest separator
    if (newlineIndex != -1 && newlineIndex < nextIndex) nextIndex = newlineIndex;
    if (commaIndex != -1 && commaIndex < nextIndex) nextIndex = commaIndex;
    if (spaceIndex != -1 && spaceIndex < nextIndex) nextIndex = spaceIndex;

    // Extract command
    String cmd = commandQueue.substring(currentIndex, nextIndex);
    cmd.trim();

    if (cmd.length() > 0) {
        CommandType cmdType = parseCommand(cmd);
        executeCommand(cmdType);
    }

    // Move to next command
    if (nextIndex < commandQueue.length()) {
        currentIndex = nextIndex + 1;
    } else {
        currentIndex = commandQueue.length();
    }

    lastCommandTime = currentTime;
    return true;
}

void AutoNavigator::executeAll() {
    if (!isEnabled) {
        Serial.println("AutoNavigator: Not enabled, skipping execution");
        return;
    }

    Serial.println("AutoNavigator: Executing all commands...");
    
    while (hasMoreCommands()) {
        executeNext();
        delay(commandDelay);  // Ensure delay between commands
    }

    Serial.println("AutoNavigator: All commands executed");
}

void AutoNavigator::clear() {
    commandQueue = "";
    currentIndex = 0;
    lastCommandTime = 0;
    Serial.println("AutoNavigator: Command queue cleared");
}

void AutoNavigator::processSerialInput() {
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();

        // Check for special commands
        if (input.startsWith("auto:load:")) {
            // Load from file: auto:load:/path/to/file.txt
            String filename = input.substring(10);
            if (loadFromFile(filename)) {
                Serial.print("AutoNavigator: Loaded commands from file: ");
                Serial.println(filename);
            } else {
                Serial.print("AutoNavigator: Failed to load file: ");
                Serial.println(filename);
            }
        } else if (input.startsWith("auto:exec:")) {
            // Execute commands from string: auto:exec:up,down,select
            String commands = input.substring(10);
            if (loadFromString(commands)) {
                Serial.println("AutoNavigator: Loaded commands from string");
            }
        } else if (input == "auto:start") {
            // Start auto navigation
            isEnabled = true;
            Serial.println("AutoNavigator: Enabled");
        } else if (input == "auto:stop") {
            // Stop auto navigation
            isEnabled = false;
            Serial.println("AutoNavigator: Disabled");
        } else if (input == "auto:clear") {
            // Clear command queue
            clear();
        } else if (input == "auto:status") {
            // Show status
            Serial.print("AutoNavigator: Enabled=");
            Serial.print(isEnabled);
            Serial.print(", Commands=");
            Serial.print(getCommandCount());
            Serial.print(", CurrentIndex=");
            Serial.print(currentIndex);
            Serial.print(", Delay=");
            Serial.print(commandDelay);
            Serial.println("ms");
        } else if (input == "auto:run") {
            // Execute all commands immediately
            executeAll();
        }
    }
}

