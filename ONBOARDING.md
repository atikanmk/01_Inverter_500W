# Project 01_Inverter_500W — Component Selection Guide
## 3-Phase BLDC Motor Inverter · 48 V / 500 W · หาซื้อได้ใน Shopee ไทย

---

## Design Parameters

| Parameter | Symbol | Value | Unit | Notes |
|-----------|--------|-------|------|-------|
| Motor Voltage (Bus) | V_bus | **48** | V | Nominal 48 V LiFePO4 / Lead-acid |
| Output Power | P_out | **500** | W | Continuous rated power |
| Motor Efficiency (est.) | η_motor | 0.85 | — | Typical BLDC efficiency |
| Inverter Efficiency (est.) | η_inv | 0.95 | — | PWM inverter typical |
| Continuous DC Current | I_dc | ~12.3 | A | P / (V × η_motor × η_inv) |
| Phase Peak Current | I_peak | ~18.4 | A | I_dc × 1.5 margin |
| **MOSFET V_ds min** | V_ds_min | **96** | V | V_bus × 2.0 safety margin |
| **MOSFET I_d min** | I_d_min | **37** | A | I_peak × 2.0 safety margin |
| Switching Frequency | f_sw | 20 | kHz | Typical BLDC inverter |
| Dead Time | t_dead | 500 | ns | Prevent shoot-through |

---

## MOSFET Selection

> ★ = แนะนำ · ✔ = ใช้ได้ · ✖ = ไม่แนะนำ  
> เกณฑ์: **V_ds ≥ 96 V** (48V × 2.0) · **I_d ≥ 37 A** (peak × 2.0 margin)

| Part | Maker | V_ds (V) | I_d (A) | R_ds(on) (mΩ) | V_gs(th) (V) | Pkg | Qg (nC) | ราคา (฿) | เลือกใช้ | หมายเหตุ |
|------|-------|----------|---------|---------------|--------------|-----|---------|-----------|----------|---------|
| **IRFB4110** | Infineon/IR | 100 | 120 | 3.7 | 2.0–4.0 | TO-220AB | 150 | 40–80 | ★ แนะนำ | Rds ต่ำมาก ทนกระแสสูง เหมาะที่สุด |
| **IRF3710** | Infineon/IR | 100 | 57 | 23 | 2.0–4.0 | TO-220AB | 71 | 25–50 | ★ แนะนำ | ราคาถูก หาง่าย เหมาะงบจำกัด |
| IRFP260N | Infineon/IR | 200 | 50 | 40 | 2.0–4.0 | TO-247AC | 97 | 60–120 | ✔ ใช้ได้ | V_ds สูง margin เยอะ แต่ Rds สูงกว่า |
| NCE3080K | NCE (TH) | 80 | 30 | 28 | 1.5–3.5 | TO-220 | 58 | 15–35 | ✔ ใช้ได้ | ราคาถูกมาก แต่ V_ds=80V margin น้อย ต้องขนาน |
| IRF540N | Infineon/IR | 100 | 33 | 77 | 2.0–4.0 | TO-220AB | 72 | 20–45 | ✔ ใช้ได้ | Rds สูง ต้องขนาน 2 ตัว/ขา |
| STP75NF75 | STMicro | 75 | 75 | 8 | 2.0–4.0 | TO-220 | 100 | 35–70 | ✔ ใช้ได้ | V_ds=75V margin 56% ควรระวัง spike |
| FDP2532 | ON Semi | 150 | 90 | 12 | 2.0–4.0 | TO-220 | 130 | 50–90 | ✔ ใช้ได้ | V_ds สูง Rds ดี แต่หายากในไทย |
| ~~IRFZ44N~~ | Infineon/IR | 55 | 49 | 17.5 | 2.0–4.0 | TO-220AB | 88 | 15–30 | ✖ ไม่แนะนำ | V_ds=55V ต่ำเกินไป! margin 14% เท่านั้น |
| ~~IRLZ44N~~ | Infineon/IR | 55 | 47 | 22 | 1.0–2.0 | TO-220AB | 90 | 20–40 | ✖ ไม่แนะนำ | V_ds=55V ต่ำเกินไปสำหรับ 48V bus |

> 🔍 **Shopee keyword**: ชื่อ Part + "MOSFET" เช่น `IRFB4110 MOSFET`

---

## Gate Driver Selection

> ใช้คู่กับ Bootstrap capacitor 100–470 nF สำหรับ High-side drive  
> สำหรับ 3-phase: ใช้ **3 ชิป** (1 ชิป/เฟส)

| Part | Maker | ประเภท | V_offset (V) | Src/Snk (A) | Delay (ns) | ราคา (฿) | Isolation | เลือกใช้ | หมายเหตุ |
|------|-------|--------|-------------|-------------|-----------|-----------|-----------|----------|---------|
| **EG3013** | EG Semi | Half-Bridge | 600 | 1.5/1.5 | 150 | 10–25 | ไม่มี | ★ แนะนำ | Clone IR2110 ราคาถูกมาก pinout เหมือนกัน หาง่ายใน Shopee |
| **IR2110** | Infineon/IR | Half-Bridge | 600 | 2.0/2.0 | 120 | 30–60 | ไม่มี | ★ แนะนำ | ยอดนิยม Bootstrap Hi-side ใช้ 3 ชิป/3-phase |
| **IR2104** | Infineon/IR | Half-Bridge | 600 | 0.6/0.6 | 680 | 20–40 | ไม่มี | ★ แนะนำ | มี SD pin เล็กกว่า เหมาะงบจำกัด |
| IR2101 | Infineon/IR | Half-Bridge | 600 | 0.2/0.2 | 680 | 15–30 | ไม่มี | ✔ ใช้ได้ | กระแสต่ำ ต้องเพิ่ม totem-pole ถ้า Qg > 50nC |
| TLP250 | Toshiba | Single Isolated | 35 | 1.5/1.5 | 500 | 25–55 | Opto 2500V | ✔ ใช้ได้ | Isolated ต้องใช้ไฟแยกต่อขา ใช้ 6 ชิป |
| TC4420 | Microchip | Lo-side Buffer | 18 | 6.0/6.0 | 45 | 40–80 | ไม่มี | ✔ ใช้ได้ | Lo-side เท่านั้น ใช้เสริม IR2110 เพิ่มกระแส |
| HCPL3120 | Broadcom | Single Isolated | 30 | 2.5/2.5 | 500 | 40–80 | Opto 1414V | ✔ ใช้ได้ | Isolated กระแสสูงกว่า TLP250 |
| DRV8302 | TI | 3-Phase IC | 60 | 1.7/2.3 | 200 | 150–300 | ไม่มี | ✔ ใช้ได้ | ไดร์ฟ 3 เฟสในชิปเดียว มี OCP ในตัว |

> 🔍 **Shopee keyword**: `EG3013 gate driver` / `IR2110 gate driver`

---

## BOM – ชุดแนะนำ (3-Phase 48V/500W)

| # | รายการ | Part Number | Qty | ราคา/หน่วย (฿) | รวม (฿) | หมายเหตุ |
|---|--------|-------------|-----|----------------|---------|---------|
| 1 | MOSFET Power Switch | IRFB4110 | 6 | 60 | **360** | 6 ตัว / 3-phase H-bridge TO-220 |
| 2 | Gate Driver IC | EG3013 | 3 | 20 | **60** | 1 ชิป/เฟส (Hi+Lo side) |
| 3 | Bootstrap Capacitor | 100nF 100V | 6 | 2 | **12** | 1 ตัว/Hi-side ทนแรงดัน ≥100V |
| 4 | Bootstrap Diode | UF4007 | 3 | 5 | **15** | Fast recovery 1A/1000V |
| 5 | Gate Resistor Hi-side | 10Ω 0.5W | 6 | 1 | **6** | จำกัดกระแส gate ลด ringing |
| 6 | Gate Resistor Lo-side | 10Ω 0.5W | 6 | 1 | **6** | จำกัดกระแส gate ลด ringing |
| 7 | Gate Pull-down | 10kΩ 0.25W | 6 | 1 | **6** | ป้องกัน gate float |
| 8 | Decoupling Cap VCC | 100nF 25V | 3 | 2 | **6** | ต่อใกล้ขา VCC ของ IC |
| 9 | Heatsink MOSFET | TO-220 fin | 6 | 20 | **120** | Rth ≤ 5 °C/W |
| 10 | Gate Protection Zener | **1N4746A** (18V 1W) | 6 | 3 | **18** | ต่อ Gate–Source คู่ขนาน clamp Vgs ≤ 18V DO-41 |
| | | | | | | |
| | **⚡ Power Supply — 48V → 12V (Gate Driver VCC)** | | | | | |
| 11 | Buck Regulator 48V→12V | **LM2576HVT-12** | 1 | 60 | **60** | TO-220, 3A, ทน 60V, Fixed 12V |
| 12 | Inductor L1 | 100µH 3A Ferrite | 1 | 35 | **35** | อิ่มตัว ≥ 3A แกน ferrite |
| 13 | Catch Diode D1 | 1N5822 Schottky | 1 | 8 | **8** | Schottky 3A/40V fast recovery |
| 14 | Input Cap Cin1 | 100µF 63V Electrolytic | 1 | 12 | **12** | ทนแรงดัน ≥ 63V ต่อขา Vin |
| 15 | Output Cap Cout1 | 100µF 25V Electrolytic | 1 | 8 | **8** | Output filter |
| | | | | | | |
| | **⚡ Power Supply — 12V → 5V (ESP32)** | | | | | |
| 16 | Buck Regulator 12V→5V | **LM2576T-5.0** | 1 | 40 | **40** | TO-220, 3A, Fixed 5V สำหรับ ESP32 |
| 17 | Inductor L2 | 100µH 3A Ferrite | 1 | 35 | **35** | ใช้เบอร์เดียวกับ L1 ได้เลย |
| 18 | Catch Diode D2 | 1N5822 Schottky | 1 | 8 | **8** | Schottky 3A/40V เหมือน D1 |
| 19 | Input Cap Cin2 | 100µF 25V Electrolytic | 1 | 8 | **8** | ต่อขา Vin LM2576T |
| 20 | Output Cap Cout2 | 100µF 10V Electrolytic | 1 | 5 | **5** | Output filter สำหรับ ESP32 |
| | | | | **รวมประมาณ** | **~846 ฿** | |

---

## Power Supply Circuit — LM2576

### Block Diagram
```
48V Bus
  ├──[LM2576HVT-12]──→ 12V ──→ Gate Driver VCC (EG3013 ×3)
  │
  └──[LM2576HVT-12]──→ 12V ──[LM2576T-5.0]──→ 5V ──→ ESP32
```

### วงจร LM2576 (ใช้เหมือนกันทั้งสองตัว)
```
         LM2576HVT / LM2576T
         ┌──────────────────┐
V_in ────┤ Vin (1)  OUT (2) ├───┬──[L 100µH]───┬─── V_out
[Cin]    │                  │   │              │
100µF    │  GND (3)  FB (4) │  [D1          [Cout
         │                  │  1N5822]      100µF]
         │   ON/OFF (5) ────┤ ต่อ GND = Always ON
         └──────────────────┘
               │
              GND
```

### ⚠️ ข้อควรระวัง
| ประเด็น | รายละเอียด |
|--------|-----------|
| **LM2576HVT-12** V_in max | 60V — ทน spike จาก 48V bus ได้ |
| **LM2576T-5.0** V_in max | 40V — ต่อจาก 12V เท่านั้น! ห้ามต่อตรงจาก 48V |
| Catch Diode | ต้องใช้ **Schottky** (1N5822) ห้ามใช้ 1N4007 (ช้าเกินไป) |
| Input Cap | Cin ของ LM2576HVT ต้องทน **≥ 63V** |

> 🔍 **Shopee keyword**: `LM2576HVT-12` / `LM2576T-5.0` / `1N5822 Schottky`

---

## Gate Drive Circuit — การต่อวงจร

### Power Supply
| Signal | Voltage | หมายเหตุ |
|--------|---------|---------|
| VDD (IC logic) | 5V หรือ 3.3V | จาก MCU supply |
| VCC (driver) | 12–15 V | drive MOSFET gate (แยกจาก 48V bus) |
| COM | GND | GND ร่วมของ Low-side และ MCU |

### Bootstrap Circuit (Hi-side)
| Node | คำอธิบาย |
|------|---------|
| VB | Bootstrap voltage = VCC + VS (floating ตาม source Hi-side MOSFET) |
| VS | ต่อกับ Source ของ Hi-side MOSFET (switching node) |
| D_boot | UF4007 — anode→VCC, cathode→VB |
| C_boot | 100–470 nF ระหว่าง VB–VS ทนแรงดัน ≥ V_bus+VCC ≈ 63V → **ใช้ 100V** |

### Gate Resistors
| Component | Value | หน้าที่ |
|-----------|-------|--------|
| R_g_on | 10 Ω | อนุกรม HO/LO → Gate MOSFET ชะลอ turn-on ลด di/dt |
| R_g_off | 0–10 Ω | แยก R สำหรับ turn-off (trade-off loss vs EMI) |
| R_gs | 10 kΩ | Gate–Source pull-down ป้องกัน gate float |

### ⚠️ Dead Time
```
Dead Time ≥ 500 ns (ตั้งใน MCU PWM timer register)
HIN และ LIN ห้าม HIGH พร้อมกัน → shoot-through = +48V shorted to GND!
```

### Decoupling
- **C_vcc** = 100nF + 10µF ต่อใกล้ขา VCC–COM ของ IC ทุกตัว
- ป้องกัน voltage dip ตอน switching

### Input Signals (จาก MCU)
| Pin | คำอธิบาย |
|-----|---------|
| HIN | High-side input — MCU PWM (active HIGH) |
| LIN | Low-side input — MCU PWM (active HIGH) |
| SD | Shutdown — active LOW/HIGH ขึ้นกับรุ่น ดู datasheet |

### Thermal
| Item | คำอธิบาย |
|------|---------|
| Heatsink | Rth ≤ (T_j_max − T_amb) / P_loss |
| P_loss/ตัว | ≈ I²×Rds×0.5 (conduction) + 0.5×Qg×Vgs×f_sw (switching) ≈ **2–5 W/ตัว** |
| Thermal paste | ทาระหว่าง MOSFET กับ heatsink ลด Rth_contact |

---

## Project Files

```
01_Inverter_500W/
├── claude_MOSFET_GateDriver_Selection_48V_500W.xlsx   ← Component selection (Excel)
└── schematic/
    ├── 3phase_power_stage.png/.svg                    ← Power stage overview
    ├── gate_driver_1phase.png/.svg                    ← Gate driver detail
    └── gate_driver_halfbridge.kicad_sch               ← KiCad v6 schematic
```

---

*Project: 01_Inverter_500W · Rev 1.1 · 2026-05-27*
