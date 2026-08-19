import requests
import time
from json import dumps
import json
import os
import argparse

import spotifyHelpers as spotifyHelpers

import sleeperHelpers as sleeperHelpers


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
    json = {'games': []}
    try:

        if (sport == 'nba'):
            response = requests.get('https://site.api.espn.com/apis/site/v2/sports/basketball/nba/scoreboard', timeout=(3.05, 10))
        elif (sport == 'wnba'):
            response = requests.get('https://site.api.espn.com/apis/site/v2/sports/basketball/wnba/scoreboard', timeout=(3.05, 10))
        elif (sport == 'nfl'):
            response = requests.get('https://site.api.espn.com/apis/site/v2/sports/football/nfl/scoreboard', timeout=(3.05, 10))
        elif (sport == 'mlb'):
            response = requests.get('https://site.api.espn.com/apis/site/v2/sports/baseball/mlb/scoreboard', timeout=(3.05, 10))
        elif (sport == 'nhl'):
            response = requests.get('https://site.api.espn.com/apis/site/v2/sports/hockey/nhl/scoreboard', timeout=(3.05, 10))
        elif (sport == 'ncaab'):
            response = requests.get('https://site.api.espn.com/apis/site/v2/sports/basketball/mens-college-basketball/scoreboard', timeout=(3.05, 10))
        elif (sport == 'ncaaf'):
            response = requests.get('https://site.api.espn.com/apis/site/v2/sports/football/college-football/scoreboard', timeout=(3.05, 10))
        elif (sport == 'fifa') :
            response = requests.get('http://site.api.espn.com/apis/site/v2/sports/soccer/fifa.world/scoreboard', timeout=(3.05, 10))
        else:
            response = {}
    except requests.exceptions.Timeout:
        print("getScores timed out", flush=True)
        return json  # or {} / cached value
    except requests.exceptions.RequestException as e:
        print(f"getScores failed: {e}", flush=True)
        return json
    events = response.json()['events']
    
    games = []
    for game in events:
        currObj = {'shortName': game['shortName'], 'status': game['status']['type']['state'], 'period': game['status']['period'] if 'period' in game['status'] else '', 'displayClock': game['status']['displayClock'], 'date': game['date'], 'competitors': []}
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
    print("Getting news ", flush=True)
    json = {'news': []}
    try:
        if (sport == 'nba'):
            response = requests.get(f'https://site.api.espn.com/apis/site/v2/sports/basketball/nba/news?limit={limit}', timeout=(3.05, 10))
        elif (sport == 'nfl'):
            response = requests.get(f'https://site.api.espn.com/apis/site/v2/sports/football/nfl/news?limit={limit}', timeout=(3.05, 10))
        else:
            response = {}
    except:
        print("The ESPN news api failed", flush=True)
        return json
    print("Successfully got the news", flush=True)
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



'''
Gets the user's current matchup using a combination of espn apis and sleeper apis
Inputs:

'''
def getSleeper(leagueID, week, sleeperObj):
    currMatchup = sleeperHelpers.getMatchup(leagueID, week, sleeperObj[0]["rosterID"])
    
    for roster in currMatchup:
        if (roster["rosterID"] == sleeperObj[0]["rosterID"]):
            sleeperObj[0]["points"] = roster["points"]
        elif (roster["rosterID"] == sleeperObj[1]["rosterID"]):
            sleeperObj[1]["points"] = roster["points"]
    return sleeperObj
    


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

    last_news = {"news": []}
    sleeperObj = None
    if mode == "breaking-news":
        last_news = getNews(league, 1)
    elif mode == "spotify":
        access_token = spotifyHelpers.authSetup()
        sleep_time = 3 # Refresh more often for spotify since the display cycles more quickly than scores
    elif mode == "sleeper":
        yearAndWeek = sleeperHelpers.getYearAndWeek()

        sleeperObj = sleeperHelpers.setup(config["sleeperUserID"], yearAndWeek["year"], yearAndWeek["week"], config["leagueID"] if config.get("leagueID") else None, CONFIG_FILE)
        if not sleeperObj:
            raise Exception("There are no leagues that are currently in season")
        if not config.get("sleeperLeague"):
            with open(config_file_path, 'r') as config_file:
                config = json.load(config_file)
         
        


    

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
            curr = getNews(league, 1)
            print(curr, flush=True)
            print(last_news)
            print(curr == last_news)
            print(len(curr["news"]) == 0, flush=True)
            if (curr == last_news or len(curr["news"]) == 0): # if we've seen this news before OR the api failed, we do not want to update last_news nor display anything
                curr = {"news": []}
            else:
                last_news = curr
        elif mode == "sleeper":
            curr = getSleeper(config["sleeperLeague"], yearAndWeek["week"], sleeperObj)
        else:
            raise Exception(f"The mode passed in was not a recognized mode: {mode}")
                
        with open(CURRENT_SCORES_FILE, 'w') as file:
                file.write(dumps(curr))

        time.sleep(sleep_time)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Retrieve information from various external APIs")
    parser.add_argument("-m", "--mode", default="", help="Mode for the backend to run in. Overrides what is present in mode.json")
    parser.add_argument("-l", "--league", default="", help="Sport for the backend to fetch. Overrides what is present in config.json")
    args = parser.parse_args()
    main(args.mode, args.league)