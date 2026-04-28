import { useState, useEffect } from "react";
import { sendMode, getMode } from "../api.ts";

export default function ModeSelector({ onModeChange }) {
  const [mode, setMode] = useState(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);

  const modes = [
    {label: "Scoreboard", value: "scoreboard"},
    {label: "Logos", value: "logos"},
    {label: "Large Logos", value: "large-logos"},
    {label: "Sports News", value: "news"},
    {label: "Breaking News", value: "breaking-news"},
    {label: "Spotify", value: "spotify"},
    {label: "Clock", value: "clock"}
  ];

  useEffect(() => {
    const fetchMode = async () => {
      console.log("Fetching the mode");
      try {
        const response = await getMode();
        setMode(response['mode']);
        onModeChange?.(response['mode']);
      } catch (err) {
        setError(err.message);
        console.log("Error");
      } finally {
        setLoading(false);
        console.log("Loading");
      }
    };
    fetchMode();
  }, []);
  

  const handleUpdate  = async () => {
    if (mode) {
      await sendMode({"mode": mode.toLowerCase()});
      onModeChange?.(mode.toLowerCase());
      console.log(`Selected mode: ${mode}`);
    } else {
      alert("Please select a mode first.");
    }
  };
  if (loading) return <p>Loading...</p>;
  if (error) return <p>Error: {error}</p>;
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
    display: "flex",
    alignItems: "center",
    justifyContent: "center",
    backgroundColor: "#f3f4f6",
  },
  card: {
    backgroundColor: "#ffffff",
    padding: "24px",
    margin: "10px",
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