import asyncio
import websockets
import json

async def test_username_not_found():
    """Test WebSocket login with non-existent username"""
    uri = "ws://localhost:8000/ws/esp32"
    
    try:
        async with websockets.connect(uri) as websocket:
            # Send login with non-existent username
            login_msg = {
                "type": "login",
                "credentials": "username:newuser999-pin:1234"
            }
            
            await websocket.send(json.dumps(login_msg))
            print(f"[SENT] {login_msg}")
            
            # Wait for response
            response = await websocket.recv()
            data = json.loads(response)
            print(f"[RECEIVED] {data}")
            
            # Verify error message
            assert data["type"] == "error", f"Expected 'error', got '{data.get('type')}'"
            assert data["message"] == "Username not found", f"Expected 'Username not found', got '{data.get('message')}'"
            print("[PASS] Test PASSED: Username not found error received correctly")
            return True
            
    except AssertionError as e:
        print(f"[FAIL] Test FAILED: {e}")
        return False
    except Exception as e:
        print(f"[ERROR] Test ERROR: {e}")
        return False

async def test_wrong_pin():
    """Test WebSocket login with existing username but wrong PIN"""
    uri = "ws://localhost:8000/ws/esp32"
    
    try:
        async with websockets.connect(uri) as websocket:
            # Send login with existing username but wrong PIN
            login_msg = {
                "type": "login",
                "credentials": "username:player1-pin:9999"
            }
            
            await websocket.send(json.dumps(login_msg))
            print(f"[SENT] {login_msg}")
            
            # Wait for response
            response = await websocket.recv()
            data = json.loads(response)
            print(f"[RECEIVED] {data}")
            
            # Verify error message
            assert data["type"] == "error", f"Expected 'error', got '{data.get('type')}'"
            assert data["message"] == "Invalid PIN", f"Expected 'Invalid PIN', got '{data.get('message')}'"
            print("[PASS] Test PASSED: Invalid PIN error received correctly")
            return True
            
    except AssertionError as e:
        print(f"[FAIL] Test FAILED: {e}")
        return False
    except Exception as e:
        print(f"[ERROR] Test ERROR: {e}")
        return False

async def test_successful_login():
    """Test WebSocket login with correct credentials"""
    uri = "ws://localhost:8000/ws/esp32"
    
    try:
        async with websockets.connect(uri) as websocket:
            # Send login with correct credentials
            login_msg = {
                "type": "login",
                "credentials": "username:player1-pin:4321"
            }
            
            await websocket.send(json.dumps(login_msg))
            print(f"[SENT] {login_msg}")
            
            # Wait for response
            response = await websocket.recv()
            data = json.loads(response)
            print(f"[RECEIVED] {data}")
            
            # Verify success message
            assert data["type"] == "login_success", f"Expected 'login_success', got '{data.get('type')}'"
            assert "session_id" in data, "Expected 'session_id' in response"
            print(f"[PASS] Test PASSED: Login successful, session: {data.get('session_id')}")
            return True
            
    except AssertionError as e:
        print(f"[FAIL] Test FAILED: {e}")
        return False
    except Exception as e:
        print(f"[ERROR] Test ERROR: {e}")
        return False

async def main():
    print("=" * 60)
    print("WebSocket Login Tests")
    print("=" * 60)
    print()
    
    print("Test 1: Username Not Found")
    print("-" * 60)
    result1 = await test_username_not_found()
    print()
    
    print("Test 2: Wrong PIN")
    print("-" * 60)
    result2 = await test_wrong_pin()
    print()
    
    print("Test 3: Successful Login")
    print("-" * 60)
    result3 = await test_successful_login()
    print()
    
    print("=" * 60)
    print("Test Summary")
    print("=" * 60)
    print(f"Username Not Found: {'PASS' if result1 else 'FAIL'}")
    print(f"Wrong PIN: {'PASS' if result2 else 'FAIL'}")
    print(f"Successful Login: {'PASS' if result3 else 'FAIL'}")
    print()
    
    if result1 and result2 and result3:
        print("[SUCCESS] All tests PASSED!")
        return 0
    else:
        print("[FAILURE] Some tests FAILED!")
        return 1

if __name__ == "__main__":
    exit_code = asyncio.run(main())
    exit(exit_code)

