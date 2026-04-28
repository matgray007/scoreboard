import requests
import time
from json import dumps
import json
import os
import argparse

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
    elif (sport == 'wnba'):
        response = requests.get('https://site.api.espn.com/apis/site/v2/sports/basketball/wnba/scoreboard')
    elif (sport == 'nfl'):
        response = requests.get('https://site.api.espn.com/apis/site/v2/sports/football/nfl/scoreboard')
    elif (sport == 'mlb'):
        response = requests.get('https://site.api.espn.com/apis/site/v2/sports/baseball/mlb/scoreboard')
    elif (sport == 'nhl'):
        response = requests.get('https://site.api.espn.com/apis/site/v2/sports/hockey/nhl/scoreboard')
    elif (sport == 'ncaab'):
        response = requests.get('https://site.api.espn.com/apis/site/v2/sports/basketball/mens-college-basketball/scoreboard')
    elif (sport == 'ncaaf'):
        response = requests.get('https://site.api.espn.com/apis/site/v2/sports/football/college-football/scoreboard')    
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
                currObj['competitors'].append({'displayName': competitor['team']['displayName'], 'abbreviation': competitor['team']['abbreviation'], 'logo': competitor['team']['logo'] if 'logo' in competitor['team'] else '', 'score': competitor['score'], 'homeAway': competitor['homeAway']})
        games.append(currObj)
    if liveOnly: # Only games that are currently in progress
        json['games'] = list(filter(lambda x: x['status'] == 'in', games))
    else: # Games that are in progress or have ended
        json['games'] = list(filter(lambda x: x['status'] == 'in' or x['status'] == 'post', games))
    return json

# TODO: Retain who has the ball, down and distance, and timeouts remaining

def getNews(sport, limit):
    if (sport == 'nba'):
        response = requests.get(f'https://site.api.espn.com/apis/site/v2/sports/basketball/nba/news?limit={limit}')
    elif (sport == 'nfl'):
        response = requests.get(f'https://site.api.espn.com/apis/site/v2/sports/football/nfl/news?limit={limit}')
    else:
        response = {}
    json = {'news': []}
    news = []
    for article in response.json()['articles']:
        if (article["type"] in ["Media", "Story"]):
            continue
        team = []
        temp = (category for category in article['categories'] if category['type'] == 'team')
        for team_temp in temp:
            team.append(team_temp['description'])
        news.append({'headline': article['headline'], 'description': article['description'], 'team': team})
    # There can be multiple teams, so just the first team is taken for simplicity. This can be changed to a list of teams if needed
    # The teams can include college teams, which are not includes in the team_config, so they will need to be filtered on the c++ side
    json['news'] = news
    return json

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



def main(mode_arg = "", league_arg = ""):
    if (len(mode_arg) == 0):
        mode_file_path = os.path.join(os.path.dirname(__file__), MODE_FILE)
        with open(mode_file_path, 'r') as mode_file:
            mode_config = json.load(mode_file)
        mode = mode_config["mode"]
    else:
        mode = mode_arg
        
    config_file_path = os.path.join(os.path.dirname(__file__), CONFIG_FILE)
    with open(config_file_path, 'r') as config_file:
        config = json.load(config_file)
    league = config["league"]
    
    if (len(league_arg) > 0):
        league = league_arg

    # Setup
    sleep_time = 5

    if mode == "spotify":
        access_token = spotifyHelpers.authSetup()
        sleep_time = 3 # Refresh more often for spotify since the display cycles more quickly than scores

    last_news = {"news": []}
    if mode == "breaking-news":
        last_news = getNews(league, 1)

    # Main loop
    while True:
        curr = {}
        if mode == "spotify":
            curr = getSong(access_token)
            
        elif mode == "scoreboard" or mode == "logos" or mode == "large-logos" :
            curr = getScores(config["liveOnly"], league)
        elif mode == "news":
            curr = getNews(league, 100)
        elif mode == "breaking-news":
            curr = getNews("nba", 1)
            if (curr == last_news):
                last_news = curr
                curr = {"news": []}
            else:
                last_news = curr
                
        with open(CURRENT_SCORES_FILE, 'w') as file:
                file.write(dumps(curr))

        time.sleep(sleep_time)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Retrieve information from various external APIs")
    parser.add_argument("-m", "--mode", default="", help="Mode for the backend to run in. Overrides what is present in mode.json")
    parser.add_argument("-l", "--league", default="", help="Sport for the backend to fetch. Overrides what is present in config.json")
    args = parser.parse_args()
    main(args.mode, args.league)