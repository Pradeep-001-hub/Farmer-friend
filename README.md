# Farmer-friend

**An intelligent, low-cost, solar-powered smart farming device** designed for small and marginal farmers in India.

Built for real-world conditions in regions like **Andhra Pradesh** (Guntur, Krishna, etc.), it uses multiple environmental factors to make smart irrigation decisions and control water pumps automatically.

---
 Problem It Solves

- Over-irrigation and water wastage
- Manual monitoring difficulties
- Unreliable electricity in rural areas
- Crop stress due to improper watering
- Lack of timely alerts during rain or dry spells

---
 Key Features

- **Multi-Factor Decision Engine** – Uses dynamic weighting of Soil Moisture, Temperature, Humidity, and Rain
- **Automatic Pump Control** via Relay
- **SMS Alerts** (no internet required) using GSM
- **Local Display** – Real-time readings on OLED
- **Solar Powered** option for off-grid operation
- **Low Power Consumption**
- **Farmer-friendly** – Simple SMS commands

---
 Hardware Components (BOM)

| Component                    | Quantity | Approx. Price (₹) | Source                  |
|-----------------------------|----------|-------------------|-------------------------|
| ESP32 DevKit                | 1        | 350               | Robu.in / Amazon       |
| Capacitive Soil Moisture Sensor | 1     | 150               | Robu.in                |
| DHT22 (Temp + Humidity)     | 1        | 150               | Robu.in                |
| DS18B20 Waterproof          | 1        | 100               | Robu.in                |
| Rain Drop Sensor            | 1        | 100               | Robu.in                |
| 5V 10A Relay Module         | 1        | 100               | Robu.in                |
| SIM800L GSM Module          | 1        | 400               | Robu.in                |
| 0.96" OLED Display (I2C)    | 1        | 250               | Robu.in                |
| TP4056 + 18650 Battery      | 1        | 200               | Local / Amazon         |
| 5W Solar Panel + Controller | 1        | 500               | Optional               |
| Jumper wires, Enclosure     | -        | 300               | -                      |
| **Total**                   | -        | **₹2,600 – 3,500** | -                      |

---

## 📊 System Architecture

![AgriGuard System Architecture](architecture.png)

*(Multi-Factor inputs → Decision Engine → Pump Control)*

---

## 🧠 Multi-Factor Algorithm

The system doesn't rely on a single sensor. It calculates an **Irrigation Score** using dynamic weights:

### Factors Considered:
- Soil Moisture (highest priority)
- Soil Temperature
- Ambient Temperature + Humidity
- Rain Detection

### Dynamic Weight Adjustment:
- Reduces soil moisture weight during rainfall
- Adjusts based on recent conditions
- Prevents over-irrigation

**Decision Logic**:
- Score < 35% → **Pump ON**
- Score > 70% → **Pump OFF**


