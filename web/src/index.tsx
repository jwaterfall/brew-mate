import { render } from "preact";
import { LocationProvider, Router, Route } from "preact-iso";

import { Header } from "./components/Header.jsx";
import { Home } from "./pages/Home/index.jsx";
import { Settings } from "./pages/Settings/index.jsx";
import { NotFound } from "./pages/_404.jsx";
import "./style.css";

export function App() {
  return (
    <LocationProvider>
      <main class=" bg-slate-900 text-white font-sans min-h-screen flex flex-col">
        <Header />
        <Router>
          <Route path="/" component={Home} />
          <Route path="/settings" component={Settings} />
          <Route default component={NotFound} />
        </Router>
      </main>
    </LocationProvider>
  );
}

render(<App />, document.getElementById("app"));
