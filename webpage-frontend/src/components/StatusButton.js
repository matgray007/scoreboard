import { useState, useEffect } from "react";
import { updateStatus, getStatus } from "../api.ts";

export default function ServiceStatus() {
  const [status, setStatus] = useState(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState(null);

  useEffect(() => {
      const fetchStatus = async () => {
        try {
          const response = await getStatus();
          if (response["scores"] == 0 && response["matrix"] == 0) {
            setStatus(true)
          } else {
            setStatus(false)
          }
        //   setStatus(response['scores']);
        } catch (err) {
          setError(err.message);
        } finally {
          setLoading(false);
        }
      };
      fetchStatus();
    }, []);

  const handleUpdate = async () => {

    setLoading(true);
    setError(null);

    try {
      const response = await updateStatus({ status: !status });
      setStatus(!status);
    } catch (err) {
      setError(err.message);
    } finally {
      setLoading(false);
    }
  };

  return (
    <div style={styles.container}>
      <div style={styles.card}>
        <h2 style={styles.title}>Service Status</h2>
        <button
          style={styles.button}
          onClick={handleUpdate}
          disabled={loading}
        >
          {loading ? "Updating..." : (status ? "Stop services" : "Start services")}
        </button>

        {error && <p style={styles.error}>Error: {error}</p>}
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
  error: {
    marginTop: "12px",
    color: "red",
    fontSize: "14px",
    textAlign: "center",
  },
};