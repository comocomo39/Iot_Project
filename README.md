# Iot_Project: Air‑System‑Monitoring IoT

Indoor‑air‑quality monitoring **and automatic ventilation control** on a resource‑constrained IoT stack.

---

## What the project does

* **Measures** CO, TVOC, CO₂‑eq, temperature, humidity and dust density with two Contiki‑NG sensor motes.
* **Predicts** short‑term temperature and air‑risk level locally with tiny Decision‑Tree models compiled via *Emlearn*.
* **Acts** through two CoAP actuators (air‑purifier & ventilation) that react to the on‑device predictions.
* **Stores & visualises** every reading in a Java cloud backend + MySQL DB, with a CLI Remote‑Control tool for live thresholds and shutdown.

## How it works  (30 s tour)

| Layer             | Components                                    | Role                                                              |
| ----------------- | --------------------------------------------- | ----------------------------------------------------------------- |
| **Edge device**   | `air_sample`, `env_sample`                    | sample environment → run ML → send JSON via CoAP                  |
| **Border Router** | standard Contiki slip‑radio                   | tunnels 6LoWPAN to IPv6 Internet                                  |
| **Cloud**         | `Main.java`, `CoapObserver`, MySQL            | registers nodes, stores time‑series, forwards alerts              |
| **Actuators**     | `air_system_actuator`, `ventilation_actuator` | drive purifier / fans with LED status                             |
| **User UI**       | `RemoteControlApp.java`                       | set risk & temp thresholds, check danger count, shut devices down |
