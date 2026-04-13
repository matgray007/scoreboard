import requests
import time
from json import dumps
import json
import os
import spotifyHelpers as spotifyHelpers


# CONSTANTS

MODE_FILE = "../mode.json"
CONFIG_FILE = "../config.json"
CURRENT_SCORES_FILE = "../currentScores.json"


'''
Gets the scores of the respective league and parses the json to extract relevant information
Inputs: 
    liveOnly: boolean indicating whether to get only live games or all games
    sport: string indicating which sport to get scores for (nfl or nba)
'''
def getScores(liveOnly, sport):
    print("getting scores...")
    if (sport == 'nba'):
        response = requests.get('https://site.api.espn.com/apis/site/v2/sports/basketball/nba/scoreboard')
    elif (sport == 'nfl'):
        response = requests.get('https://site.api.espn.com/apis/site/v2/sports/football/nfl/scoreboard')
    elif (sport == 'ncaab'):
        response = requests.get('https://site.api.espn.com/apis/site/v2/sports/basketball/mens-college-basketball/scoreboard')
    else:
        response = {}
    events = response.json()['events']
    json = {'games': []}
    games = []
    for game in events:
        currObj = {'shortName': game['shortName'], 'status': game['status']['type']['state'], 'period': game['status']['period'], 'displayClock': game['status']['displayClock'], 'date': game['date'], 'competitors': []}
        # status- pre: prior to game started; in: game is live; post: game has ended
        for competition in game['competitions']:
            for competitor in competition['competitors']:
                currObj['competitors'].append({'displayName': competitor['team']['displayName'], 'abbreviation': competitor['team']['abbreviation'], 'logo': competitor['team']['logo'], 'score': competitor['score'], 'homeAway': competitor['homeAway']})
        games.append(currObj)
    if liveOnly: # Only games that are currently in progress
        json['games'] = list(filter(lambda x: x['status'] == 'in', games))
    else: # Games that are in progress or have ended
        json['games'] = list(filter(lambda x: x['status'] == 'in' or x['status'] == 'post', games))
    return json

# TODO: Retain who has the ball, down and distance, and timeouts remaining

def getNews(sport):
    if (sport == 'nba'):
        response = requests.get('https://site.api.espn.com/apis/site/v2/sports/basketball/nba/news')
    elif (sport == 'nfl'):
        response = requests.get('https://site.api.espn.com/apis/site/v2/sports/football/nfl/news')
    else:
        response = {}
    json = {'news': []}
    news = []
    for article in response.json()['articles']:
        team = []
        temp = (category for category in article['categories'] if category['type'] == 'team')
        for team_temp in temp:
            team.append(team_temp['description'])
        news.append({'headline': article['headline'], 'description': article['description'], 'team': team})
    # There can be multiple teams, so just the first team is taken for simplicity. This can be changed to a list of teams if needed
    # The teams can include college teams, which are not includes in the team_config, so they will need to be filtered on the c++ side
    json['news'] = news
    return news


'''
Gets the song currently playing on a user's Spotify account *requires setup of Spotify developer*
Inputs: 
    liveOnly: boolean indicating whether to get only live games or all games
    sport: string indicating which sport to get scores for (nfl or nba)
'''
def getSong(access_token):
    currSong = spotifyHelpers.getSong(access_token)
    if currSong["currently_playing"] is None:
        spotifyHelpers.refreshToken()
        currSong = spotifyHelpers.getSong(access_token)
    return currSong



def main():
    mode_file_path = os.path.join(os.path.dirname(__file__), MODE_FILE)
    with open(mode_file_path, 'r') as mode_file:
        mode_config = json.load(mode_file)
    config_file_path = os.path.join(os.path.dirname(__file__), CONFIG_FILE)
    with open(config_file_path, 'r') as config_file:
        config = json.load(config_file)

    mode = mode_config["mode"]
    # Setup
    sleep_time = 20

    if mode == "spotify":
        access_token = spotifyHelpers.authSetup()
        sleep_time = 5 # Refresh more often for spotify since the display cycles more quickly than scores



    # Main loop
    while True:
        curr = {}
        if mode == "spotify":
            curr = getSong(access_token)
            
        elif mode == "scoreboard" or mode == "logos" or mode == "large-logos" :
            curr = getScores(config["liveOnly"], config["league"])
        elif mode == "news":
            curr = getNews(config["league"])
        with open(CURRENT_SCORES_FILE, 'w') as file:
                file.write(dumps(curr))

        time.sleep(sleep_time)

if __name__ == "__main__":
    main()