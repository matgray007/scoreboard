// npm run start
import { useState } from "react";
import './App.css';
import ModeSelector from './components/ModeSelector';
import StatusButton from './components/StatusButton';
import ShutdownButton from './components/ShutdownButton';
import SportsOptions from './components/SportsOptions';

function App() {
  const [mode, setMode] = useState(null);
  return (
    <div className="App">
      <StatusButton />
      <ModeSelector onModeChange={setMode} />
      <SportsOptions mode={mode} />
      <ShutdownButton />

    </div>
  );
}

export default App;
