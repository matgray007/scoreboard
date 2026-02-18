
const API_BASE_URL = "http://10.0.0.6:8000"

export interface ModeResponse {
    mode: string;
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