
const API_BASE_URL = "http://10.0.0.6:8000"

export interface ModeResponse {
    mode: string;
}

export async function getStatus() {
    const res = await fetch(`${API_BASE_URL}/status`, {
        method: "GET",
        headers: {
            "Content-Type": "application/json",
        },
    });
    if (!res.ok) {
        console.error("error getting statuses api.ts")
        throw new Error("Failed to get service statuses")
    }
    console.log("Returning get status");
    return res.json();
}

export async function updateStatus(body: { status: boolean }) {
    console.log("Updating status ", JSON.stringify(body));
  const res = await fetch(`${API_BASE_URL}/status`, {
    method: "PATCH",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify(body),
  });

  if (!res.ok) {
    throw new Error("Failed to update service statuses");
  }

  return res.json();
}

export async function sendPiAction(body: {action: string}) {
    const res = await fetch(`${API_BASE_URL}/rasp-pi`, {
        method: "POST",
        headers: {
            "Content-Type": "application/json",
        },
        body: JSON.stringify(body),
    });
}

export async function sendMode(mode: object): Promise<ModeResponse> {
    const response = await fetch(`${API_BASE_URL}/mode`,
        {
            method: "PATCH",
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify(mode),
        }
    );
    if (!response.ok) {
        throw new Error(`Error while sending the mode ${await response.text()}`);
    }
    return response.json();
}

export async function getMode(): Promise<ModeResponse> {
    const response = await fetch(`${API_BASE_URL}/mode`,
        {
            method: "GET",
            headers: {
                "Content-Type": "application/json"
            }
        }
    );
    if (!response.ok) {
        throw new Error(`Error while getting the mode ${await response.text()}`);
    }
    return response.json();
}

export async function getConfig() {
    const response = await fetch(`${API_BASE_URL}/config`, {
        method: "GET",
        headers: {
            "Content-Type": "application/json"
            }
        }
    );
    if (!response.ok) {
        throw new Error(`Error while getting the config ${await response.text()}`);
    }
    return response.json();
}

export async function updateConfig(body: {liveOnly: boolean, league: string}) {
    console.log("Update config api.ts");
    const response = await fetch(`${API_BASE_URL}/config`, {
        method: "PATCH",
        headers: {
            "Content-Type": "application/json"
            },
        body: JSON.stringify(body),
        });
    if (!response.ok) {
        throw new Error(`Error while getting the config ${await response.text()}`);
    }
    console.log("End of updateconfig api.ts");
    return response.json();
}