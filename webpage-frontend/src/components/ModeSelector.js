import { useState } from "react";
import { sendMode } from "../api.ts";

export default function ModeSelector() {
  const [mode, setMode] = useState(null);

  const modes = [
    {label: "Scoreboard", value: "scoreboard"},
    {label: "Logos", value: "logos"},
    {label: "Large Logos", value: "large-logos"},
    {label: "Spotify", value: "spotify"},
    {label: "Clock", value: "clock"}
  ];

  const handleUpdate  = async () => {
    // TODO: This is where the api call will go to update the matrix mode on the backend
    if (mode) {
      await sendMode({"mode": mode.toLowerCase()});
      console.log(`Selected mode: ${mode}`);
    } else {
      alert("Please select a mode first.");
    }
  };

  return (
    <div style={styles.container}>
      <div style={styles.card}>
        <h2 style={styles.title}>Select a Mode</h2>

        <div style={styles.options}>
          {modes.map((m) => (
            <label key={m.value} style={styles.label}>
              <input
                type="radio"
                name="mode"
                value={m.value}
                checked={mode === m.value}
                onChange={() => setMode(m.value)}
                style={styles.radio}
              />
              {m.label}
            </label>
          ))}
        </div>

        <button style={styles.button} onClick={handleUpdate}>
          Update Matrix Mode
        </button>
      </div>
    </div>
  );
}

const styles = {
  container: {
    minHeight: "100vh",
    display: "flex",
    alignItems: "center",
    justifyContent: "center",
    backgroundColor: "#f3f4f6",
  },
  card: {
    backgroundColor: "#ffffff",
    padding: "24px",
    borderRadius: "12px",
    width: "320px",
    boxShadow: "0 10px 25px rgba(0,0,0,0.1)",
  },
  title: {
    textAlign: "center",
    marginBottom: "16px",
  },
  options: {
    marginBottom: "20px",
  },
  label: {
    display: "flex",
    alignItems: "center",
    marginBottom: "8px",
    cursor: "pointer",
  },
  radio: {
    marginRight: "8px",
  },
  button: {
    width: "100%",
    padding: "10px",
    borderRadius: "8px",
    border: "none",
    backgroundColor: "#2563eb",
    color: "white",
    fontSize: "16px",
    cursor: "pointer",
  },
};