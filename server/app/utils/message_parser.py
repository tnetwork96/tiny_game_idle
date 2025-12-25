"""
String pattern message parser for ESP32 communication.
All messages use format: key1:value1-*-key2:value2-*-key3:value3
"""


def parse_message(message: str) -> dict:
    """
    Parse string pattern message into dictionary.
    
    Args:
        message: String in format "key1:value1-*-key2:value2"
    
    Returns:
        Dictionary with parsed key-value pairs
    
    Example:
        >>> parse_message("type:login-*-username:player1-*-pin:4321")
        {"type": "login", "username": "player1", "pin": "4321"}
    """
    data = {}
    if not message:
        return data
    
    pairs = message.split("-*-")
    
    for pair in pairs:
        if ":" in pair:
            # Split only on first colon (in case value contains colon)
            key, value = pair.split(":", 1)
            data[key] = value
    
    return data


def build_message(**kwargs) -> str:
    """
    Build string pattern message from key-value pairs.
    
    Args:
        **kwargs: Key-value pairs to include in message
    
    Returns:
        String in format "key1:value1-*-key2:value2"
    
    Example:
        >>> build_message(type="login_success", session_id="abc-123", message="Success")
        "type:login_success-*-session_id:abc-123-*-message:Success"
    """
    pairs = []
    for key, value in kwargs.items():
        # Convert value to string
        value_str = str(value) if value is not None else ""
        pairs.append(f"{key}:{value_str}")
    
    return "-*-".join(pairs)

