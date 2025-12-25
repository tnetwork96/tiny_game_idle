import requests
import json

BASE_URL = "http://localhost:8000"

def test_username_not_found():
    """Test HTTP API login with non-existent username"""
    print("Test 1: HTTP API - Username Not Found")
    print("-" * 60)
    
    try:
        response = requests.post(
            f"{BASE_URL}/api/login",
            json={
                "username": "newuser456",
                "pin": "5678"
            },
            timeout=5
        )
        
        print(f"Status Code: {response.status_code}")
        print(f"Response: {response.json()}")
        
        assert response.status_code == 401, f"Expected 401, got {response.status_code}"
        assert response.json()["detail"] == "Username not found", f"Expected 'Username not found', got '{response.json().get('detail')}'"
        print("[PASS] Test PASSED: Username not found error received correctly")
        return True
        
    except AssertionError as e:
        print(f"✗ Test FAILED: {e}")
        return False
    except Exception as e:
        print(f"✗ Test ERROR: {e}")
        return False

def test_wrong_pin():
    """Test HTTP API login with existing username but wrong PIN"""
    print("\nTest 2: HTTP API - Wrong PIN")
    print("-" * 60)
    
    try:
        response = requests.post(
            f"{BASE_URL}/api/login",
            json={
                "username": "player1",
                "pin": "9999"
            },
            timeout=5
        )
        
        print(f"Status Code: {response.status_code}")
        print(f"Response: {response.json()}")
        
        assert response.status_code == 401, f"Expected 401, got {response.status_code}"
        assert response.json()["detail"] == "Invalid PIN", f"Expected 'Invalid PIN', got '{response.json().get('detail')}'"
        print("[PASS] Test PASSED: Invalid PIN error received correctly")
        return True
        
    except AssertionError as e:
        print(f"✗ Test FAILED: {e}")
        return False
    except Exception as e:
        print(f"✗ Test ERROR: {e}")
        return False

def test_successful_login():
    """Test HTTP API login with correct credentials"""
    print("\nTest 3: HTTP API - Successful Login")
    print("-" * 60)
    
    try:
        response = requests.post(
            f"{BASE_URL}/api/login",
            json={
                "username": "player1",
                "pin": "4321"
            },
            timeout=5
        )
        
        print(f"Status Code: {response.status_code}")
        print(f"Response: {response.json()}")
        
        assert response.status_code == 200, f"Expected 200, got {response.status_code}"
        assert response.json()["status"] == "success", f"Expected 'success', got '{response.json().get('status')}'"
        print("[PASS] Test PASSED: Login successful")
        return True
        
    except AssertionError as e:
        print(f"✗ Test FAILED: {e}")
        return False
    except Exception as e:
        print(f"✗ Test ERROR: {e}")
        return False

def test_register_and_login():
    """Test registration then login"""
    print("\nTest 4: HTTP API - Register and Login")
    print("-" * 60)
    
    try:
        # Register new user
        register_response = requests.post(
            f"{BASE_URL}/api/register",
            json={
                "username": "testuser123",
                "pin": "1234"
            },
            timeout=5
        )
        
        print(f"Registration Status: {register_response.status_code}")
        print(f"Registration Response: {register_response.json()}")
        
        assert register_response.status_code == 200, f"Expected 200, got {register_response.status_code}"
        print("[PASS] Registration successful")
        
        # Try to login with new user
        login_response = requests.post(
            f"{BASE_URL}/api/login",
            json={
                "username": "testuser123",
                "pin": "1234"
            },
            timeout=5
        )
        
        print(f"Login Status: {login_response.status_code}")
        print(f"Login Response: {login_response.json()}")
        
        assert login_response.status_code == 200, f"Expected 200, got {login_response.status_code}"
        print("[PASS] Login after registration successful")
        return True
        
    except AssertionError as e:
        print(f"✗ Test FAILED: {e}")
        return False
    except Exception as e:
        print(f"✗ Test ERROR: {e}")
        return False

def main():
    print("=" * 60)
    print("HTTP API Login Tests")
    print("=" * 60)
    print()
    
    results = []
    results.append(test_username_not_found())
    results.append(test_wrong_pin())
    results.append(test_successful_login())
    results.append(test_register_and_login())
    
    print("\n" + "=" * 60)
    print("Test Summary")
    print("=" * 60)
    print(f"Username Not Found: {'PASS' if results[0] else 'FAIL'}")
    print(f"Wrong PIN: {'PASS' if results[1] else 'FAIL'}")
    print(f"Successful Login: {'PASS' if results[2] else 'FAIL'}")
    print(f"Register and Login: {'PASS' if results[3] else 'FAIL'}")
    print()
    
    if all(results):
        print("[SUCCESS] All tests PASSED!")
        return 0
    else:
        print("[FAILURE] Some tests FAILED!")
        return 1

if __name__ == "__main__":
    exit_code = main()
    exit(exit_code)

