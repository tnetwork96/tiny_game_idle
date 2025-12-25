# String Protocol Template - ESP32 to Server Communication

## Core Principle

**All communication between ESP32 and server uses STRING format, NOT JSON.**

All messages must follow a consistent pattern that can be easily parsed by splitting.

## Pattern Format

### Basic Pattern
```
key1:value1-*-key2:value2-*-key3:value3
```

### Separator
- **Field Separator:** `-*-` (dash-asterisk-dash)
- **Key-Value Separator:** `:` (colon)

### Rules
1. **No JSON:** Never use JSON format for messages
2. **Consistent Pattern:** All messages follow `key:value-*-key:value` pattern
3. **Order Independent:** Keys can be in any order (but recommended order for consistency)
4. **No Spaces:** No spaces around separators (except in values if needed)
5. **Empty Values:** Use empty string `""` for missing optional values
6. **Special Characters:** If value contains `-*-` or `:`, must be escaped or encoded

## Message Format Examples

### 1. Login Message (ESP32 → Server)

**Format:**
```
type:login-*-username:xxxx-*-pin:xxxx
```

**Example:**
```
type:login-*-username:player1-*-pin:4321
```

**Parsing (ESP32 side):**
```cpp
String message = "type:login-*-username:player1-*-pin:4321";
// Split by "-*-" to get pairs
// Parse each pair by ":" to get key-value
```

**Parsing (Server side - Python):**
```python
message = "type:login-*-username:player1-*-pin:4321"
pairs = message.split("-*-")
data = {}
for pair in pairs:
    key, value = pair.split(":", 1)  # Split only on first colon
    data[key] = value
# Result: {"type": "login", "username": "player1", "pin": "4321"}
```

### 2. Login Success Response (Server → ESP32)

**Format:**
```
type:login_success-*-session_id:xxxx-*-message:xxxx
```

**Example:**
```
type:login_success-*-session_id:abc-123-def-456-*-message:Authentication successful
```

**Parsing (ESP32 side):**
```cpp
String response = "type:login_success-*-session_id:abc-123-def-456-*-message:Authentication successful";
// Parse to extract session_id and message
```

### 3. Login Error Response (Server → ESP32)

**Format:**
```
type:error-*-message:xxxx
```

**Example (Username Not Found):**
```
type:error-*-message:Username not found
```

**Example (Invalid PIN):**
```
type:error-*-message:Invalid PIN
```

**Example (Account Inactive):**
```
type:error-*-message:Account inactive
```

### 4. Ping Message (ESP32 → Server)

**Format:**
```
type:ping
```

**Example:**
```
type:ping
```

### 5. Pong Response (Server → ESP32)

**Format:**
```
type:pong
```

**Example:**
```
type:pong
```

### 6. Chat Message (ESP32 → Server)

**Format:**
```
type:chat_message-*-message:xxxx-*-receiver:xxxx
```

**Example:**
```
type:chat_message-*-message:Hello friend!-*-receiver:alice
```

### 7. Chat Acknowledgment (Server → ESP32)

**Format:**
```
type:chat_ack-*-message:xxxx-*-message_id:xxxx
```

**Example:**
```
type:chat_ack-*-message:Message received-*-message_id:12345
```

### 8. Game Action (ESP32 → Server)

**Format:**
```
type:game_action-*-action:xxxx-*-game_type:xxxx-*-data:xxxx
```

**Example:**
```
type:game_action-*-action:start_game-*-game_type:billiard-*-data:level1
```

### 9. Game Response (Server → ESP32)

**Format:**
```
type:game_response-*-action:xxxx-*-status:xxxx-*-data:xxxx
```

**Example:**
```
type:game_response-*-action:start_game-*-status:received-*-data:game_id_123
```

### 10. Friend List Request (ESP32 → Server)

**Format:**
```
type:get_friends-*-username:xxxx
```

**Example:**
```
type:get_friends-*-username:player1
```

### 11. Friend List Response (Server → ESP32)

**Format:**
```
type:friends_list-*-count:xxxx-*-friends:friend1,friend2,friend3
```

**Example:**
```
type:friends_list-*-count:4-*-friends:alice,bob,charlie,diana
```

**Note:** For lists, use comma-separated values within the value field.

### 12. Add Friend Request (ESP32 → Server)

**Format:**
```
type:add_friend-*-username:xxxx-*-friend_username:xxxx
```

**Example:**
```
type:add_friend-*-username:player1-*-friend_username:eve
```

### 13. Add Friend Response (Server → ESP32)

**Format:**
```
type:add_friend_response-*-status:success-*-message:xxxx
```

**Example (Success):**
```
type:add_friend_response-*-status:success-*-message:Friend added successfully
```

**Example (Failed):**
```
type:add_friend_response-*-status:failed-*-message:User not found
```

## Parsing Functions

### ESP32 Side (C++)

#### Parse String Message
```cpp
// Simple parsing function
struct MessageData {
    String type;
    StringMap fields;  // Map of key-value pairs (use your own StringMap implementation)
};

MessageData parseMessage(String message) {
    MessageData data;
    
    // Split by field separator
    int startIndex = 0;
    int separatorIndex = message.indexOf("-*-");
    
    while (separatorIndex != -1 || startIndex < message.length()) {
        String pair;
        if (separatorIndex == -1) {
            pair = message.substring(startIndex);
            startIndex = message.length();
        } else {
            pair = message.substring(startIndex, separatorIndex);
            startIndex = separatorIndex + 3;  // Skip "-*-"
            separatorIndex = message.indexOf("-*-", startIndex);
        }
        
        // Parse key:value
        int colonIndex = pair.indexOf(":");
        if (colonIndex != -1) {
            String key = pair.substring(0, colonIndex);
            String value = pair.substring(colonIndex + 1);
            
            if (key == "type") {
                data.type = value;
            } else {
                data.fields[key] = value;
            }
        }
    }
    
    return data;
}
```

#### Get Value from Parsed Message
```cpp
String getValue(MessageData& data, const String& key, const String& defaultValue = "") {
    if (data.fields.find(key) != data.fields.end()) {
        return data.fields[key];
    }
    return defaultValue;
}
```

#### Example Usage
```cpp
String response = "type:login_success-*-session_id:abc-123-*-message:Success";
MessageData msg = parseMessage(response);

if (msg.type == "login_success") {
    String sessionId = getValue(msg, "session_id");
    String message = getValue(msg, "message");
    
    Serial.print("Session ID: ");
    Serial.println(sessionId);
}
```

### Server Side (Python)

#### Parse String Message
```python
def parse_message(message: str) -> dict:
    """
    Parse string message into dictionary.
    
    Args:
        message: String in format "key1:value1-*-key2:value2"
    
    Returns:
        Dictionary with parsed key-value pairs
    """
    data = {}
    pairs = message.split("-*-")
    
    for pair in pairs:
        if ":" in pair:
            key, value = pair.split(":", 1)  # Split only on first colon
            data[key] = value
    
    return data

# Example usage
message = "type:login-*-username:player1-*-pin:4321"
data = parse_message(message)
# Result: {"type": "login", "username": "player1", "pin": "4321"}

msg_type = data.get("type")
username = data.get("username")
pin = data.get("pin")
```

#### Build String Message
```python
def build_message(**kwargs) -> str:
    """
    Build string message from key-value pairs.
    
    Args:
        **kwargs: Key-value pairs to include in message
    
    Returns:
        String in format "key1:value1-*-key2:value2"
    """
    pairs = []
    for key, value in kwargs.items():
        pairs.append(f"{key}:{value}")
    
    return "-*-".join(pairs)

# Example usage
message = build_message(
    type="login_success",
    session_id="abc-123-def",
    message="Authentication successful"
)
# Result: "type:login_success-*-session_id:abc-123-def-*-message:Authentication successful"
```

## Implementation Guidelines

### ESP32 Side

#### 1. Sending Messages
```cpp
// Instead of JSON:
// String json = "{\"type\":\"login\",\"username\":\"player1\",\"pin\":\"4321\"}";

// Use string pattern:
String message = "type:login-*-username:player1-*-pin:4321";
webSocket.sendTXT(message);
```

#### 2. Receiving Messages
```cpp
void handleMessage(String message) {
    MessageData msg = parseMessage(message);
    
    if (msg.type == "login_success") {
        String sessionId = getValue(msg, "session_id");
        authenticated = true;
        // Handle success
    }
    else if (msg.type == "error") {
        String errorMsg = getValue(msg, "message");
        // Handle error
    }
    else if (msg.type == "pong") {
        // Handle pong
    }
}
```

#### 3. Helper Functions
```cpp
// Build message helper
String buildMessage(const String& type, const StringMap& fields) {
    String message = "type:" + type;
    for (auto& pair : fields) {
        message += "-*-";
        message += pair.first;
        message += ":";
        message += pair.second;
    }
    return message;
}

// Example
StringMap fields;
fields["username"] = "player1";
fields["pin"] = "4321";
String loginMsg = buildMessage("login", fields);
// Result: "type:login-*-username:player1-*-pin:4321"
```

### Server Side (Python/FastAPI)

#### 1. Receiving Messages
```python
@router.websocket("/ws/esp32")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    
    try:
        # Receive string message
        message = await websocket.receive_text()
        
        # Parse message
        data = parse_message(message)
        msg_type = data.get("type")
        
        if msg_type == "login":
            username = data.get("username")
            pin = data.get("pin")
            # Verify credentials...
            
            # Send response
            response = build_message(
                type="login_success",
                session_id=session_id,
                message="Authentication successful"
            )
            await websocket.send_text(response)
        
        elif msg_type == "ping":
            response = build_message(type="pong")
            await websocket.send_text(response)
    
    except Exception as e:
        error_msg = build_message(
            type="error",
            message=str(e)
        )
        await websocket.send_text(error_msg)
```

#### 2. Sending Messages
```python
# Instead of JSON:
# response = {"type": "login_success", "session_id": "abc", "message": "Success"}
# await websocket.send_json(response)

# Use string pattern:
response = build_message(
    type="login_success",
    session_id="abc-123-def",
    message="Authentication successful"
)
await websocket.send_text(response)
```

## Message Type Reference

### Request Messages (ESP32 → Server)

| Type | Format | Required Fields | Optional Fields |
|------|--------|----------------|-----------------|
| `login` | `type:login-*-username:xxx-*-pin:xxx` | `username`, `pin` | - |
| `ping` | `type:ping` | - | - |
| `chat_message` | `type:chat_message-*-message:xxx-*-receiver:xxx` | `message`, `receiver` | - |
| `game_action` | `type:game_action-*-action:xxx-*-game_type:xxx` | `action`, `game_type` | `data` |
| `get_friends` | `type:get_friends-*-username:xxx` | `username` | - |
| `add_friend` | `type:add_friend-*-username:xxx-*-friend_username:xxx` | `username`, `friend_username` | - |

### Response Messages (Server → ESP32)

| Type | Format | Required Fields | Optional Fields |
|------|--------|----------------|-----------------|
| `login_success` | `type:login_success-*-session_id:xxx-*-message:xxx` | `session_id`, `message` | - |
| `error` | `type:error-*-message:xxx` | `message` | `error_code` |
| `pong` | `type:pong` | - | - |
| `chat_ack` | `type:chat_ack-*-message:xxx` | `message` | `message_id` |
| `game_response` | `type:game_response-*-action:xxx-*-status:xxx` | `action`, `status` | `data` |
| `friends_list` | `type:friends_list-*-count:xxx-*-friends:xxx,xxx,xxx` | `count`, `friends` | - |
| `add_friend_response` | `type:add_friend_response-*-status:xxx-*-message:xxx` | `status`, `message` | - |

## Special Cases

### Empty Values
If a field is optional and not provided, omit it entirely:
```
// Good
type:ping

// Also acceptable (empty value)
type:ping-*-data:
```

### Lists/Arrays
For lists, use comma-separated values within the value:
```
type:friends_list-*-count:4-*-friends:alice,bob,charlie,diana
```

### Values Containing Special Characters
If value contains `-*-` or `:`, consider:
1. **URL Encoding:** Encode special characters
2. **Base64 Encoding:** For complex data
3. **Alternative Separator:** Use different separator for that field

**Example with URL encoding:**
```
type:chat_message-*-message:Hello%20friend%21-*-receiver:alice
```

### Multi-line Values
For multi-line values, use newline character `\n`:
```
type:message-*-content:Line1\nLine2\nLine3
```

## Migration from JSON

### Before (JSON)
```cpp
// ESP32
String json = "{\"type\":\"login\",\"username\":\"player1\",\"pin\":\"4321\"}";
webSocket.sendTXT(json);

// Server
data = json.loads(message)
msg_type = data["type"]
```

### After (String Pattern)
```cpp
// ESP32
String message = "type:login-*-username:player1-*-pin:4321";
webSocket.sendTXT(message);

// Server
data = parse_message(message)
msg_type = data["type"]
```

## Advantages

1. **No JSON Library:** ESP32 doesn't need ArduinoJson library
2. **Smaller Memory:** String parsing uses less RAM than JSON
3. **Faster Parsing:** Simple string split is faster than JSON parsing
4. **Easier Debugging:** Human-readable format in Serial Monitor
5. **Simpler Code:** Less complex parsing logic

## Disadvantages

1. **No Nested Structures:** Cannot represent complex nested objects
2. **Type Safety:** All values are strings (need manual conversion)
3. **Special Characters:** Need encoding for special characters in values
4. **No Arrays:** Must use comma-separated strings for lists

## Best Practices

1. **Always include `type` field:** First field should always be `type`
2. **Consistent field order:** Use same order for same message types
3. **Validate on receive:** Always check `type` field exists
4. **Handle missing fields:** Use default values for optional fields
5. **Error handling:** Always handle parsing errors gracefully
6. **Documentation:** Document all message types and fields

## Testing

### Test Cases

#### Valid Messages
```
type:ping
type:login-*-username:player1-*-pin:4321
type:login_success-*-session_id:abc-123-*-message:Success
```

#### Edge Cases
```
type:ping-*-data:                    // Empty value
type:message-*-text:Hello-*-World    // Value without key (invalid)
type:login-*-username:player1        // Missing required field (pin)
```

### Validation Function
```cpp
bool validateMessage(String message) {
    // Check if message contains type field
    if (message.indexOf("type:") != 0) {
        return false;
    }
    
    // Check if separators are valid
    int count = 0;
    int index = 0;
    while ((index = message.indexOf("-*-", index)) != -1) {
        count++;
        index += 3;
    }
    
    // Should have at least one separator for multi-field messages
    // Or no separator for single-field messages
    return true;
}
```

## File Locations

- **ESP32 Parsing:** `src/websocket_client.cpp` - `parseMessage()` function
- **Server Parsing:** `server/app/websocket/esp32_handler.py` - `parse_message()` function
- **Message Builders:** Create helper files for building messages

## Summary

- **Format:** `key1:value1-*-key2:value2-*-key3:value3`
- **Separator:** `-*-` between fields, `:` between key and value
- **No JSON:** All messages are plain strings
- **Simple Parsing:** Split by `-*-`, then split each pair by `:`
- **Type Field:** Always include `type` as first field
- **Consistent:** Use same pattern for all messages

