import requests
import time
from json import dumps
import json
import os
from urllib.parse import urlencode



def get_user_auth_url(client_id, redirect_uri, scope):
    auth_url = 'https://accounts.spotify.com/authorize'
    params = {
        'client_id': client_id,
        'response_type': 'code',
        'redirect_uri': redirect_uri,
        'scope': scope
    }
    return f"{auth_url}?{urlencode(params)}"

def get_access_token_from_code(client_id, client_secret, code, redirect_uri):
    """Exchange authorization code for access token"""
    token_url = 'https://accounts.spotify.com/api/token'
    
    response = requests.post(token_url, data={
        'grant_type': 'authorization_code',
        'code': code,
        'redirect_uri': redirect_uri,
        'client_id': client_id,
        'client_secret': client_secret,
    })
    return response.json()

def refresh_access_token(client_id, client_secret, refresh_token):
    """Refresh an expired access token"""
    token_url = 'https://accounts.spotify.com/api/token'
    
    response = requests.post(token_url, data={
        'grant_type': 'refresh_token',
        'refresh_token': refresh_token,
        'client_id': client_id,
        'client_secret': client_secret,
    })
    
    return response.json()

def getSong(access_token):
    headers = {
        "Authorization": f"Bearer {access_token}"
    }
    response = requests.get('https://api.spotify.com/v1/me/player/queue', headers=headers)
    
    if response.status_code == 401:
        return None  # Token expired
    elif response.status_code == 403:
        print("Error: Insufficient permissions or forbidden")
        return None
    
    response_data = response.json()
    trimmed_data = {"currently_playing": {"song": response_data["currently_playing"]["name"], "artist": response_data["currently_playing"]["artists"][0]["name"], "album_art": response_data["currently_playing"]["album"]["images"][0]["url"]} if response_data["currently_playing"] else None}
    
    return trimmed_data



    




config_file_path = os.path.join(os.path.dirname(__file__), 'config.json')
with open(config_file_path, 'r') as config_file:
    config = json.load(config_file)
secrets_file_path = os.path.join(os.path.dirname(__file__), 'secrets.json')
with open(secrets_file_path, 'r') as secrets_file:
    secrets = json.load(secrets_file)

client_id = secrets["spotify"]["client_id"]
client_secret = secrets["spotify"]["client_secret"]
redirect_uri = "https://127.0.0.1:8000/callback"  # Must match Spotify Dashboard setting
scope = "user-read-playback-state user-read-currently-playing"

token_file = 'spotify_token.json'

if os.path.exists(token_file):
    with open(token_file, 'r') as f:
        token_data = json.load(f)
    
    # Try to refresh the token
    new_token_data = refresh_access_token(client_id, client_secret, token_data['refresh_token'])
    access_token = new_token_data['access_token']
    
    # Update stored token
    if 'refresh_token' not in new_token_data:
        new_token_data['refresh_token'] = token_data['refresh_token']
    
    with open(token_file, 'w') as f:
        json.dump(new_token_data, f)
else:
    # First time setup - need user authorization
    auth_url = get_user_auth_url(client_id, redirect_uri, scope)
    print(f"\nPlease visit this URL to authorize the application:\n{auth_url}\n")
    
    # Copy the authorization code from the redirect URL after granting permissions
    auth_code = input("Enter the authorization code from the URL: ")
    
    token_data = get_access_token_from_code(client_id, client_secret, auth_code, redirect_uri)
    access_token = token_data['access_token']
    
    # Save tokens for future use
    with open(token_file, 'w') as f:
        json.dump(token_data, f)


# Main loop to continuously fetch current song and update JSON file
while True:
    currSong = getSong(access_token)
    if currSong is None:
        # Token expired, refresh it
        with open(token_file, 'r') as f:
            token_data = json.load(f)
        
        new_token_data = refresh_access_token(client_id, client_secret, token_data['refresh_token'])
        access_token = new_token_data['access_token']
        
        if 'refresh_token' not in new_token_data:
            new_token_data['refresh_token'] = token_data['refresh_token']
        
        with open(token_file, 'w') as f:
            json.dump(new_token_data, f)
        
        currSong = getSong(access_token)

    if currSong:
        with open('currentScores.json', 'w') as file:
            file.write(dumps(currSong))

    time.sleep(20)