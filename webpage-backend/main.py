# fastapi dev main.py --host 0.0.0.0

from fastapi import FastAPI, Body
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
import subprocess
import json
import os
import signal


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

class ModeUpdate(BaseModel):
    mode: str

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
MODE_FILE = os.path.join(BASE_DIR, "..", "mode.json")

@app.get("/")
def read_root():
    return {"Hello": "World"}

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
    
