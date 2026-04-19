# RGB Matrix Display Board
A real-time display powered by a Raspberry Pi that translates data from external APIs into a formatted visualization on the matrix board.

![Large Logos with Laptop](images/large_logos_with_laptop.jpeg)

## Author
Matthew Gray

GitHub: https://github.com/matgray007

Email: matt.r.g@outlook.com

LinkedIn: https://www.linkedin.com/in/mattgray7/

## Table of Contents
- [Overview](#overview)
- [Features](#features)
  - [Frontend](#frontend)
  - [Backend](#backend)
  - [Web App](#web-app)
  - [Video Demo](#video-demo)
  - [APIs Used](#apis-used)
- [Architecture](#architecture)
  - [Hardware Architecture](#hardware-architecture)
  - [Software Architecture](#software-architecture)
- [Setup Instructions](#setup-instructions)
  - [Hardware Setup](#hardware-setup)
  - [Software Setup](#software-setup)
  - [Running the Matrix](#running-the-matrix)
- [Configuration](#configuration)
- [Learnings](#learnings)
- [Future Improvements](#future-improvements)

---

## Overview
The RGB Matrix Display Board is a Raspberry Pi-powered system that renders real-time data from external APIs onto a low-resolution LED matrix. This project takes advantage of publicly-available APIs (namely ESPN and Spotify) to display live music, sports game scores, and other dynamic information in a format templated for the respective information

The system transform raw API JSON responses into compact visual elements ideal for an LED matrix (i.e. scrolling text, icons, etc.). A modular architecture allows for new external sources and display modes to be easily added with minimal changes required.

This project emphasizes real-time processing, hardware-software integration, and aesthetic but efficient display functionality.

## Features

This project has many different modes ranging in both fidelity and application scope. The matrix display scripts (matrix frontend) controls how the data retrieved is displayed. The data retrieval script (matrix backend) controls what is displayed. Both matrix frontend and backend have distinct modes that control their functionality. See the [Configuration](#configuration) section for setting/changing modes.

Images of the application running in the various different modes can be found in the [Images](#images) section below.

### Frontend

As of 4/5/2026, the matrix display modes include the following:
- Scoreboard: Displays live and final sports scores from the current day.
- News: Displays recent sports-related news.
- Spotify Currently Playing: Displays the current song's album cover with scrolling song name and artist name(s)
- Clock: Current time (based on Raspberry Pi's current time) with ambient background

### Backend

The matrix backend script allows for some additional constraints within the above display modes
- Scoreboard
    - Live and Final: Retrieves all games today that are live or finished
    - Live Only: Filters out games that are not currently live
    - Favorite team only: Filters out all games that do not have the favorite team playing
    - Leagues: For each of the above, the league type must be selected to display games from that league. The leagues include:
        - NFL: National Football League
        - NBA: National Basketball Association
        - NCAAB: National Collegiate Athletic Association Men's Basketball

### Web App

In addition to the matrix frontend and backend, a simple React web app with Fast API backend was created for configuration management and general Raspberry Pi control. This was created to eliminate the need to ssh into the Pi for general matrix use.

- Image of the webpage:

![Webpage](images/webpage.png)

### Video Demo

Click on the below image to view a youtube video of the matrix display board's different modes:
[![Hyperlink to video](images/clock.jpeg)](https://youtu.be/b0iZcqvFudc)

Click on the below image to view a youtube video of the matrix display board's large logo mode:
[![Hyperlink to video](images/large_logos_closeup2.jpeg)](https://youtu.be/xod1xzB__D0)


### APIs Used

This project uses the rpi-rgb-led-matrix library by Henner Zeller,  
licensed under the GNU General Public License v2.0 (or later).

See: http://www.gnu.org/licenses/gpl-2.0.txt

All modifications and usage in this project comply with the terms of the GPL license.

This project uses data from Spotify and ESPN APIs but is not affiliated with, endorsed by, or sponsored by Spotify or ESPN.

---
Henner Zeller's rpi-rgb-led-matrix library can be accessed [here](https://github.com/hzeller/rpi-rgb-led-matrix).

ESPN does not maintain official documentation for their public API. A GitHub repository outlining the available endpoints can be accessed [here](https://gist.github.com/akeaswaran/b48b02f1c94f873c6655e7129910fc3b).

For the Spotify functionality, Spotify for Developers was used. Their Web API documentation can be viewed [here](https://developer.spotify.com/documentation/web-api)

## Architecture

### Hardware Architecture

The hardware used for setup of this project includes:

- Raspberry Pi 4 Model B with power supply
- [64x32 RGB LED Matrix](https://www.adafruit.com/product/2278?srsltid=AfmBOoojB0MHMiDCVwrGxwuXMxjh9mwOA3fRDIbY4-guGwiUdotE89o-)
- [RGB Matrix Bonnet for Raspberry Pi](https://www.adafruit.com/product/3211?srsltid=AfmBOooZFZea9TqcYhKiVhNOVhHUmpka0WX1viSn1NGUvAPfMHScY6NS)
- [Socket Riser Headers](https://www.adafruit.com/product/4079?srsltid=AfmBOoq-9Pl8re24S9C_WstJWneUwBN8Y2hODJGWKcyMhp-Skhn0-d8b) (optional, but recommended)
- [5V 4A Power Supply](https://www.adafruit.com/product/1466)
- [3D Printed Stand](https://learn.adafruit.com/stream-deck-controlled-rgb-message-panel-using-adafruit-io/3d-printing)

The Raspberry Pi is capable of powering the matrix under light load on its own, but performance issues are likely if running without the second power supply for the bonnet. Most likely, the reds will display but the greens and blues will not.

### Software Architecture

#### General Workflow

```mermaid
flowchart TD 
    subgraph Control_Plane["Control Plane"]
        UI[React Web App]
        API[FastAPI Backend] 
    end 

    subgraph Raspberry_Pi["Raspberry Pi"]
        PY[Python Data Fetcher]
        JSON[currentScores.json]
        CPP[C++ Display Renderer]
    end

    EXT[External API]
    LED[LED Matrix]

    UI <--> API

    API <-->|bash commands / config updates| PY
    API -->|SIGHUP / restart| CPP

    PY -->|fetch data| EXT
    PY -->|write| JSON
    CPP -->|read| JSON
    CPP -->|render| LED
```

#### Data Flow
```mermaid
flowchart LR 
    CONFIG[Mode + Config Files]
    PY[Python Script]
    JSON[currentScores.json]
    CPP[C++ Renderer]
    LED[LED Matrix]

    CONFIG --> PY
    CONFIG --> CPP
    PY -->|poll + sleep | JSON
    JSON -->|shared state| CPP
    CPP -->|loop + display delay| LED
```

The data flow diagram is included to give more detail on how the runtime functionality works. The sleep after polling is included to ensure requests fall within the external APIs' rates.

## Setup Instructions

### Hardware Setup

Please view [these wiring guidelines](matrix/wiring.md) for wiring concerns not covered in this section. This section will only go over wiring for setup with the above hardware.

![Wiring Image](images/wiring.jpeg)

Pictured above is the wiring setup used for the creation of this application. 

The left power supply is plugged into the Matrix bonnet and the power supply on the bottom is plugged into the Pi. The matrix connects to the bonnet's screw terminals. There is an extra end to this cord in case one Pi is being used to power two boards. The data ribbon also connects directly into the bonnet. 

### Software Setup

This system has been tested on Debian GNU/Linux 12. It should be Linux-agnostic, however it has not been tested on other versions. Python 3.11.2 and pip 23.0.1. 

Cloning and compiling of the Zeller matrix API:

`git submodule update --init --recursive`  
`cd matrix`  
`make -C lib`  

#### Dependencies
Dependencies for running the matrix can be viewed in [this file](.dependencies). Additional dependencies required for use of the Zeller API can be found in [this file](matrix/utils/README.md).

### Running the Matrix

Once the necessary dependencies have been installed and both repositories have been cloned (this one and Zeller's rpi-rgb-led-matrix library), I would recommend running one of Zeller's demos as a sanity check. These demos can be viewed [here](matrix/examples-api-use/README.md). The matrix frontend code can be compiled by running `cd frontend && make`. Once it has finished compiling, the executable can be run with a variety of flags. A majority of these flags are hardware-specific, so vary as needed.


| Flag | Meaning | Values Used with Above Hardware |
| -------- | -------- | -------- |
| --led-rows=# | The number of rows available on the LED board | 32 |
| --led-cols=# | The number of columns available on the LED board | 64 |
| --led-limit-refresh=# | Rate at which the board will update. 0 means no limits. Defaulted to 0. | 60 |
| --led-slowdown-gpio=# | Needed for faster pis to ensure accurate speed. Might need to play with this number. | 2 |
| --led-gpio-mapping=[string] | The name of the GPIO mapping used. | adafruit-hat |
| -d [string] | The mode that the matrix will display in. | This value varies. See the [table below](#configuration) for available options. |
| -o | Favorite team only. | No value provided |

A [mode.json.copy](mode.json.copy) and a [config.json.copy](config.json.copy) have been included for your convenience. Copy the contents into mode.json and config.json respectively and alter as needed.

The first time running the backend with spotify, you will need to authenticate your account. The prompts in the terminal will walk you through what needs to be done.




### Running the website

Once packages have been installed, the frontend can be run with `cd webpage-frontend && npm start`. In a new terminal, the backend can be run with `cd webpage-backend && fastapi dev main.py --host 0.0.0.0`.

## Configuration

 ### Matrix Frontend

 The matrix frontend utilizes multiple methods to control the configuration. The mode.json and config.json files are initially read to retrieve the mode it is to run in and then any additional configurations, respectively. However, while running the matrix frontend from the command line, any flags passed in will override what the executable reads from the files. The various modes are outlined here:



 | Flag | Value | Description |
| -------- | -------- | -------- |
| -d | scoreboard | Basic scoreboard display. Retrieves the live games and cycles through them one at a time displaying the current score and time. |
| -d | logos | The same functionality as scoreboard mode except the team logos are displayed instead of the abbreviations. |
| -d | large-logos | The same functionality as logos except the logos take up a majority of the screen real estate. |
| -d | news | Scroll recent news across the screen and display corresponding logo and clock. |
| -d | spotify | Queries the connected Spotify account and displays the live song, artist, and album cover. |
| -d | clock | Displays the Pi's current time with an ambient background. |
| -o | N/A | Only display the scores of the game containing the favorite team listed in the config. Only valid in a sports-based mode. |


## Learnings

- Hardware-Software integration is non-trivial and ultimately requires the software to be designed to fit within the hardware limitations. 

- Real-time systems require trade offs. Specifically, the latency and rate-limiting fight one another for importance, so this trade off must be chosen consciously. 

- Data transformation was ultimately the most tedious and most difficult aspect. The data is fetched as a JSON, but then forming this object into a display that is not only informational, but also aesthetic, is very challenging

## Future Improvements

This project has infinite possible depth and breadth. I hope to explore both of these directions as much as possible. The changes and additions most likely to occur next are improvements to what already exist before expanding in new directions. Here is a far from comprehensive list of improvements:

- Webpage styling
- "No songs being played currently"
- Toggleable icon-from-URL functionality 
    - Currently, the matrix frontend tries to retrieve the icon from the logos directory. If the logo is not present, then it fetches the logo from the URL present in the API response.
- Favorite team-animations
    - Namely touchdowns, field goals, sacks, etc.
    - Some example API responses for an NFL game are present in [this](ESPNResponses/) directory.
- NFL position on field
    - A thin line 10/12 pixels long that is white(?) up to where the team with the ball currently is
    - Maybe turns red while in the redzone?
- Partial Fill() funciton to fill (clear) only part of the screen.
    - So we don't have to draw the image to the matrix every loop and can just redraw the stuff that is changing

## Images

- Scoreboard mode depicting a live NBA game:
![Scoreboard](images/scoreboard.jpeg)

- Logos mode also depicting a live NBA game:
![Logos](images/logos1.jpeg)

- Large logos mode:
![Large Logos](images/large_logos1.jpeg)

- A close up of the large logos mode:
![Large Logos Closeup](images/large_logos_closeup1.jpeg)

- Spotify mode showing the album cover, song name, and artist name of the currently playing song:
![Spotify](images/spotify.jpeg)

- Clock mode displaying the current time with ambient, blue lines racing across the background:
![Clock](images/clock.jpeg)
