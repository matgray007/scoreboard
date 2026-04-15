# fastapi dev main.py --host 0.0.0.0

from fastapi import FastAPI, Body
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
import subprocess
import json
import os
import signal
import time


app = FastAPI(
    servers = [
        {"url": "http://10.0.0.6:8000", "description": "LED Matrix's webpage backend server"}
    ]
)



app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # OK for development
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
MODE_FILE = os.path.join(BASE_DIR, "..", "mode.json")
CONFIG_FILE = os.path.join(BASE_DIR, "..", "config.json")

class ModeUpdate(BaseModel):
    mode: str

class StatusUpdate(BaseModel):
    status: bool

class PiAction(BaseModel):
    action: str

class ConfigUpdate(BaseModel):
    liveOnly: bool
    league: str


'''
Test API to ensure backend is working
'''
@app.get("/")
def read_root():
    return {"Hello": "World"}


'''
Retrieves the status of the matrix's frontend and backend services
'''
@app.get("/status")
def get_status():
    scores_result = subprocess.run(['sudo', 'systemctl', 'status', 'get-scores.service'])
    matrix_result = subprocess.run(['sudo', 'systemctl', 'status', 'write-matrix.service'])
    return {
        "message": "Successfully got the service statuses",
        "scores": scores_result.returncode,
        "matrix": matrix_result.returncode
    }

'''
Updates the status of the matrix's frontend and backend services
'''

@app.patch("/status")
def update_status(body: StatusUpdate  = Body(...)):
    if body.status:
        status = "start"
    else:
        status = "stop"
    scores_result = subprocess.run(['sudo', 'systemctl', status, 'get-scores.service'])
    if (status == "start"):
        time.sleep(0.5) # Half a second delay added to ensure scores are retrieved prior to writing to the matrix
    matrix_result = subprocess.run(['sudo', 'systemctl', status, 'write-matrix.service'])
    return {
        "message": "Attempted to update the service statuses",
        "scores": scores_result.returncode,
        "matrix": matrix_result.returncode
    }


'''
This function is a bit of a "Catch all" where I am handling specific raspberry pi functions that enhance the overall experience but aren't necessarily needed for the program
'''
@app.post("/rasp-pi")
def rasp_pi_status(body: PiAction = Body(...)):
    if body.action == "shutdown":
        subprocess.run(['sudo', 'systemctl', 'stop', 'get-scores.service'])
        subprocess.run(['sudo', 'systemctl', 'stop', 'write-matrix.service'])
        time.sleep(1)
        action_result = subprocess.run(['sudo', 'shutdown', '-h', 'now'])


'''
Retrieves the mode that the matrix's services are currently running in
'''
@app.get("/mode")
def get_mode():
    with open(MODE_FILE, 'r') as f:
        data_dict = json.load(f)
    return data_dict

'''
Updates the mode that the matrix's services are currently running in
'''
@app.patch("/mode")
def update_mode(body: ModeUpdate = Body(...)):
    with open(MODE_FILE, 'w') as f:
        json.dump({'mode': body.mode}, f)
    matrix_result = subprocess.run(
        ['sudo', 'systemctl', 'kill', '-s', 'HUP', 'write-matrix.service'],
        capture_output=True, text=True
    ) # Sends sighup to matrix service

    python_result = subprocess.run(
        ["sudo", "systemctl", "restart", "get-scores.service"],
        capture_output=True,
        text=True,
        timeout=30
    )

    # TODO: Add try/except where it is needed here


    return {"message": "Config updated successfully"} # TODO: return some of the results in this json to handle on the frontend


'''
Retrieves the config that the matrix's services are currently running on
'''
@app.get("/config")
def get_config():
    with open(CONFIG_FILE, 'r') as f:
        config_dict = json.load(f)
    return config_dict


'''
Updates the config that the matrix's services are currently running on
'''
@app.patch("/config")
def update_config(body: ConfigUpdate = Body(...)):
    print("Here we are in the update_config", flush=True)
    curr_config = get_config()
    curr_config['liveOnly'] = body.liveOnly
    curr_config['league'] = body.league
    with open(CONFIG_FILE, 'w') as f:
        json.dump(curr_config, f)

    matrix_result = subprocess.run(
        ['sudo', 'systemctl', 'kill', '-s', 'HUP', 'write-matrix.service'],
        capture_output=True, text=True
    ) # Sends sighup to matrix service

    python_result = subprocess.run(
        ["sudo", "systemctl", "restart", "get-scores.service"],
        capture_output=True,
        text=True,
        timeout=30
    )
