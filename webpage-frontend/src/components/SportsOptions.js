import { useState, useEffect } from "react";
import { updateConfig, getConfig } from "../api.ts";

export default function ConfigSelector({ mode }) {
  const [liveOnly, setLiveOnly] = useState(null);
  const [league, setLeague] = useState(null);
  // TODO: Add favoriteOnly as an option to change in here too
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);
  useEffect(() => {
    const fetchConfig = async () => {
    console.log("Fetching the config");
    try {
      const response = await getConfig();
      setLiveOnly(response["liveOnly"]);
      setLeague(response["league"]);
    } catch (err) {
      setError(err.message);
      console.log("Error fetching config");
    } finally {
      setLoading(false);
      console.log("Done loading");
    }
  };
  fetchConfig();
  }, []);
  if (mode == "large-logos" || mode == "logos" || mode == "scoreboard" || mode == "news") {
    

    const leagues = [
        { label: "NBA", value: "nba" },
        { label: "NFL", value: "nfl" },
        { label: "NHL", value: "nhl" },
        { label: "MLB", value: "mlb" },
        { label: "NCAA Basketball", value: 'ncaab' },
        { label: "NCAA Football", value: 'ncaaf' },
        // { label: "WNBA", value: "wnba" } // Something with the wnba is breaking things both frontend and backend...
    ];

    

    const handleUpdate = async () => {
        if (league !== null && liveOnly !== null) {
        await updateConfig({ liveOnly: liveOnly, league: league });
        console.log(`Updated config: liveOnly=${liveOnly}, league=${league}`);
        } else {
        alert("Please select a league first.");
        }
    };

    if (loading) return <p>Loading...</p>;
    if (error) return <p>Error: {error}</p>;

    return (
        <div style={styles.container}>
        <div style={styles.card}>
            <h2 style={styles.title}>Config Settings</h2>

            <div style={styles.section}>
            <h3 style={styles.sectionTitle}>League</h3>
            <div style={styles.options}>
                {leagues.map((l) => (
                <label key={l.value} style={styles.label}>
                    <input
                    type="radio"
                    name="league"
                    value={l.value}
                    checked={league === l.value}
                    onChange={() => setLeague(l.value)}
                    style={styles.radio}
                    />
                    {l.label}
                </label>
                ))}
            </div>
            </div>

            <div style={styles.section}>
            <h3 style={styles.sectionTitle}>Live Games Only</h3>
            <label style={styles.toggleLabel}>
                <div
                style={{
                    ...styles.toggle,
                    backgroundColor: liveOnly ? "#2563eb" : "#d1d5db",
                }}
                onClick={() => setLiveOnly(!liveOnly)}
                >
                <div
                    style={{
                    ...styles.toggleKnob,
                    transform: liveOnly ? "translateX(20px)" : "translateX(2px)",
                    }}
                />
                </div>
                <span style={styles.toggleText}>{liveOnly ? "Enabled" : "Disabled"}</span>
            </label>
            </div>

            <button style={styles.button} onClick={handleUpdate}>
            Update Config
            </button>
        </div>
        </div>
    );
  }
  
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
  section: {
    marginBottom: "20px",
  },
  sectionTitle: {
    fontSize: "14px",
    fontWeight: "600",
    color: "#374151",
    marginBottom: "10px",
  },
  options: {
    display: "flex",
    flexDirection: "column",
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
  toggleLabel: {
    display: "flex",
    alignItems: "center",
    cursor: "pointer",
    gap: "12px",
  },
  toggle: {
    width: "44px",
    height: "24px",
    borderRadius: "12px",
    position: "relative",
    cursor: "pointer",
    transition: "background-color 0.2s ease",
  },
  toggleKnob: {
    position: "absolute",
    top: "2px",
    width: "20px",
    height: "20px",
    borderRadius: "50%",
    backgroundColor: "white",
    boxShadow: "0 1px 3px rgba(0,0,0,0.2)",
    transition: "transform 0.2s ease",
  },
  toggleText: {
    fontSize: "14px",
    color: "#374151",
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