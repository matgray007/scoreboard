# Scoreboard
Collect scores of sports games from ESPN api and display them on an RGB LED monitor using a Raspberry Pi

# Dependencies
`sudo apt-get install libjsoncpp-dev`

# Venv
`python3 -m venv .venv`
`source .venv/bin/activate`
`pip3 install requests`

# Retrieving scores
`python3 getScores.py`

# Read scores to LED Matrix
`git submodule update --init --recusive`
`cd matrix`
`make -C lib`
`cd ..`
`make`
`./sendScores.exe`

