import requests
import time
from json import dumps
import json
import os
'''
Gets the scores of the respective league and parses the json to extract relevant information
Inputs: 
    liveOnly: boolean indicating whether to get only live games or all games
    sport: string indicating which sport to get scores for (nfl or nba)
'''
def getScores(liveOnly, sport):
    if (sport == 'nba'):
        response = requests.get('https://site.api.espn.com/apis/site/v2/sports/basketball/nba/scoreboard')
    elif (sport == 'nfl'):
        response = requests.get('https://site.api.espn.com/apis/site/v2/sports/football/nfl/scoreboard')
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
                currObj['competitors'].append({'displayName': competitor['team']['displayName'], 'score': competitor['score'], 'homeAway': competitor['homeAway']})
        games.append(currObj)
    if liveOnly: # Only games that are currently in progress
        json['games'] = list(filter(lambda x: x['status'] == 'in', games))
    else: # Games that are in progress or have ended
        json['games'] = list(filter(lambda x: x['status'] == 'in' or x['status'] == 'post', games))
    with open('currentScores.json', 'w') as file:
        file.write(dumps(json))
    return

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
    with open('currentNews.json', 'w') as file:
        file.write(dumps(json))
    return news



config_file_path = os.path.join(os.path.dirname(__file__), 'config.json')
with open(config_file_path, 'r') as config_file:
    config = json.load(config_file)
while True:
    getScores(config["liveOnly"], config["league"])
    # getNews(config["league"])

    time.sleep(20)
