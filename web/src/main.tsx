import React from "react";
import ReactDOM from "react-dom/client";
import App from "./App";
import Lights from "./Lights";
import "./styles.css";

const page = window.location.pathname.replace(/\/+$/, "") || "/";
const Root = page.endsWith("/lights") ? Lights : App;

ReactDOM.createRoot(document.getElementById("root")!).render(
  <React.StrictMode>
    <Root />
  </React.StrictMode>,
);
