import requests

response = requests.get("http://192.168.1.7:8080/api/notifications/3")
print("=== API RESPONSE ===")
print(response.text[:2000])  # First 2000 chars
