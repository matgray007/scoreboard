import requests
import json
import os
from json import dumps

SLEEPER_URL = "https://api.sleeper.app/v1"

AVATAR_URL = "https://sleepercdn.com/avatars"

ESPN_URL = "https://site.api.espn.com/apis/site/v2/sports/football/nfl/scoreboard?limit=1"



'''
Returns the current NFL year
'''
def getYearAndWeek():
    res = requests.get(ESPN_URL)
    sb = res.json()
    try:
        return {
            "year": sb["season"]["year"], "week": sb["week"]["number"]
        }
    except():
        raise Exception("There was an issue with the espn endpoint while trying to get the current week and year")
'''
Given a sleeper username, return the user_id
Inputs:
    username: string of the Sleeper username or userID
'''
def getUserInfo(username):
    res = requests.get(f"{SLEEPER_URL}/user/{username}")
    res.raise_for_status()
    user = res.json()
    return {"userID": user["user_id"], "avatar": user["avatar"], "username": user["username"]}

'''
Gets all of the leagues a user is in for a given year
Inputs:
    userID: string of the sleeper user ID retrieved from the /user/ endpoint
    year: current year being queried
'''
def getLeagueIDs(userID, year):
    # nfl is currently hardcoded because that is the only sport the sleeper APIs support at the moment
    res = requests.get(f"{SLEEPER_URL}/user/{userID}/leagues/nfl/{year}")
    res.raise_for_status()
    ret = [
        {"name": league["name"], "leagueID": league["league_id"], "status": league["status"]}
        for league in res.json()
    ]
    return ret

'''

'''
def getRosters(leagueID):
    res = requests.get(f"{SLEEPER_URL}/league/{leagueID}/rosters")
    res.raise_for_status()
    ret = [
        {"userID": roster["owner_id"], "rosterID": roster["roster_id"]}
        for roster in res.json()
    ]
    return ret

def getMatchup(leagueID, week, myRosterID):
    res = requests.get(f"{SLEEPER_URL}/league/{leagueID}/matchups/{week}")
    res.raise_for_status()

    ret = []
    myRoster = next((roster for roster in res.json() if roster["roster_id"] == myRosterID), None)
    matchupID = myRoster["matchup_id"]
    ret.append({"rosterID": myRoster["roster_id"], "points": myRoster["points"]})

    oppRoster = next((roster for roster in res.json() if roster["matchup_id"] == matchupID and roster["roster_id"] != myRosterID), None)

    ret.append({"rosterID": oppRoster["roster_id"], "points": oppRoster["points"]})

    print(ret)
    return ret

def aggregateInfo(matchup, rosters, myUser):
    myStuff = {"rosterID": matchup[0]["rosterID"], "username": myUser["username"], "points": matchup[0]["points"], "avatar": f"{AVATAR_URL}/{myUser['avatar']}" if myUser.get("avatar") else "https://i.pravatar.cc/100"}

    oppRoster = next((roster for roster in rosters if roster["rosterID"] == matchup[1]["rosterID"]), None)
    oppUserID = oppRoster["userID"]
    oppUser = getUserInfo(oppUserID)

    theirStuff = {"rosterID": matchup[1]["rosterID"], "username": oppUser["username"], "points": matchup[1]["points"], "avatar": f"{AVATAR_URL}/{oppUser['avatar']}" if not None else "https://i.pravatar.cc/100"}

    return [myStuff, theirStuff]

def setup(userID, year, week,   leagueID, CONFIG_FILE):
    user = getUserInfo(userID)
    userID = user["userID"]
    if (leagueID is None):
        leagues = getLeagueIDs(userID, year)
        leagueID = next((league["leagueID"] for league in leagues if league["status"] == "in_season"), None)
        if leagueID is None:
            return {}
        else:
            config_file_path = os.path.join(os.path.dirname(__file__), CONFIG_FILE)
            with open(config_file_path, 'r') as config_file:
                config = json.load(config_file)
            config["sleeperLeague"] = leagueID
            with open(config_file_path, 'w') as file:
                file.write(dumps(config))

    

    rosters = getRosters(leagueID)
    myRosterID = next((roster for roster in rosters if roster["userID"] == userID), None)
    print(myRosterID)
    matchup = getMatchup(leagueID, week, myRosterID["rosterID"])

    return aggregateInfo(matchup, rosters, user)

if __name__ == "__main__":
    
    ret = setup("711486439187120128", "2026", "1312109158987735040")
    print(ret)

    print("Pretending like we are waiting for the loop")

    currMatchup = getMatchup("1312109158987735040", "1", ret[0]["rosterID"])

    for roster in currMatchup:
        if (roster["rosterID"] == ret[0]["rosterID"]):
            ret[0]["points"] = roster["points"]
        elif (roster["rosterID"] == ret[1]["rosterID"]):
            ret[1]["points"] = roster["points"]
    print("After updatein")

