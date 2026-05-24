# -*- coding: utf-8 -*-
"""
Generate KiCad v6 schematic (.kicad_sch)
48V/500W Motor Inverter – Gate Driver one half-bridge
Import: EasyEDA Pro > File > Import > KiCad
"""
import os
from uuid import uuid4

OUT = os.path.dirname(os.path.abspath(__file__))
def uid(): return str(uuid4())

lines = []
def emit(s=""): lines.append(s)

# ═══════════════════════════════════════════════════════════════════
# KiCad v6 conventions
#   - coords in mm, positive Y = DOWN
#   - pin (at X Y angle): connection point at (X,Y), body points in `angle`
#     angle 0=right, 90=up(-y), 180=left, 270=down(+y)
#   - component rotation: 0=no rotation, 90=CCW 90°
#   - rotated pin new pos: angle=90 → (x,y)→(-y,x)
# ═══════════════════════════════════════════════════════════════════

# ── wire / label helpers ─────────────────────────────────────────────────────
def wire(x1,y1,x2,y2):
    emit(f'  (wire (pts (xy {x1:.3f} {y1:.3f}) (xy {x2:.3f} {y2:.3f}))')
    emit(f'    (stroke (width 0) (type default)) (uuid "{uid()}"))')

def junc(x,y):
    emit(f'  (junction (at {x:.3f} {y:.3f}) (diameter 0) (color 0 0 0 0) (uuid "{uid()}"))')

def net_label(x,y,name,angle=0):
    emit(f'  (label "{name}" (at {x:.3f} {y:.3f} {angle})')
    emit(f'    (effects (font (size 1.27 1.27))) (uuid "{uid()}"))')

def note(x,y,txt,size=1.0):
    emit(f'  (text "{txt}" (at {x:.3f} {y:.3f} 0)')
    emit(f'    (effects (font (size {size} {size}))) (uuid "{uid()}"))')

def pwr_sym(lib_id,ref,x,y,angle=0):
    """Place a power symbol (GND / VCC / +48V)."""
    emit(f'  (symbol (lib_id "{lib_id}") (at {x:.3f} {y:.3f} {angle}) (unit 1)')
    emit(f'    (in_bom yes) (on_board yes) (fields_autoplaced)')
    emit(f'    (uuid "{uid()}")')
    emit(f'    (property "Reference" "{ref}" (at {x:.3f} {y+3:.3f} 0)')
    emit(f'      (effects (font (size 1.27 1.27)) (hide yes)))')
    emit(f'    (property "Value" "{lib_id.split(":")[1]}" (at {x:.3f} {y-3:.3f} 0)')
    emit(f'      (effects (font (size 1.27 1.27))))')
    emit(f'    (pin "1" (uuid "{uid()}"))')
    emit(f'  )')

def place_comp(lib_id, ref, value, x, y, angle, pins, extra_props=None):
    """Place a component. pins = number of pins (for uuid gen)."""
    emit(f'  (symbol (lib_id "{lib_id}") (at {x:.3f} {y:.3f} {angle}) (unit 1)')
    emit(f'    (in_bom yes) (on_board yes) (dnp no) (fields_autoplaced)')
    emit(f'    (uuid "{uid()}")')
    emit(f'    (property "Reference" "{ref}" (at {x:.3f} {y-4:.3f} {angle})')
    emit(f'      (effects (font (size 1.27 1.27))))')
    emit(f'    (property "Value" "{value}" (at {x:.3f} {y+4:.3f} {angle})')
    emit(f'      (effects (font (size 1.27 1.27))))')
    if extra_props:
        for k,v in extra_props.items():
            emit(f'    (property "{k}" "{v}" (at {x:.3f} {y:.3f} 0)')
            emit(f'      (effects (font (size 1.27 1.27)) (hide yes)))')
    for i in range(1, pins+1):
        emit(f'    (pin "{i}" (uuid "{uid()}"))')
    emit(f'  )')

# ── pin position helper (after component rotation) ───────────────────────────
def rotxy(dx, dy, angle):
    """Rotate (dx,dy) by angle degrees CCW."""
    import math
    a = math.radians(angle)
    return (dx*math.cos(a) - dy*math.sin(a),
            dx*math.sin(a) + dy*math.cos(a))

def pin_pos(cx, cy, dx, dy, angle):
    """Absolute schematic position of a pin after component rotation."""
    rdx, rdy = rotxy(dx, dy, angle)
    return round(cx+rdx, 3), round(cy+rdy, 3)

# ═══════════════════════════════════════════════════════════════════
# lib_symbols  (all symbols defined inline – no external library needed)
# ═══════════════════════════════════════════════════════════════════
LIB = """\
  (lib_symbols

    ;── Resistor  (pin1 top y=-3.81, pin2 bottom y=+3.81) ──────────────────
    (symbol "Cust:R" (pin_names (offset 0)) (in_bom yes) (on_board yes)
      (symbol "R_0_1"
        (rectangle (start -1.016 2.794) (end 1.016 -2.794)
          (stroke (width 0.254)) (fill (type background)))
        (polyline (pts (xy 0 -2.794) (xy 0 -3.81))
          (stroke (width 0)) (fill (type none)))
        (polyline (pts (xy 0  2.794) (xy 0  3.81))
          (stroke (width 0)) (fill (type none))))
      (symbol "R_1_1"
        (pin passive line (at 0 -3.81 90) (length 0)
          (name "~" (effects (font (size 1.27 1.27))))
          (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 0  3.81 270) (length 0)
          (name "~" (effects (font (size 1.27 1.27))))
          (number "2" (effects (font (size 1.27 1.27)))))))

    ;── Capacitor  (pin1 top y=-2.032, pin2 bottom y=+2.032) ───────────────
    (symbol "Cust:C" (pin_names (offset 0.254)) (in_bom yes) (on_board yes)
      (symbol "C_0_1"
        (polyline (pts (xy -1.524 -0.508) (xy 1.524 -0.508))
          (stroke (width 0.508)) (fill (type none)))
        (polyline (pts (xy -1.524  0.508) (xy 1.524  0.508))
          (stroke (width 0.508)) (fill (type none)))
        (polyline (pts (xy 0 -0.508) (xy 0 -2.032))
          (stroke (width 0)) (fill (type none)))
        (polyline (pts (xy 0  0.508) (xy 0  2.032))
          (stroke (width 0)) (fill (type none))))
      (symbol "C_1_1"
        (pin passive line (at 0 -2.032 90) (length 0)
          (name "~" (effects (font (size 1.27 1.27))))
          (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 0  2.032 270) (length 0)
          (name "~" (effects (font (size 1.27 1.27))))
          (number "2" (effects (font (size 1.27 1.27)))))))

    ;── Diode  horizontal  (A pin1 left x=-3.81, K pin2 right x=+3.81) ─────
    (symbol "Cust:D" (pin_names (offset 0)) (in_bom yes) (on_board yes)
      (symbol "D_0_1"
        (polyline (pts (xy -1.524 0) (xy 1.524 0))
          (stroke (width 0)) (fill (type none)))
        (polyline
          (pts (xy -1.524 -1.27) (xy -1.524 1.27) (xy 1.524 0) (xy -1.524 -1.27))
          (stroke (width 0.254)) (fill (type background)))
        (polyline (pts (xy 1.524 -1.27) (xy 1.524 1.27))
          (stroke (width 0.508)) (fill (type none))))
      (symbol "D_1_1"
        (pin passive line (at -3.81 0 0) (length 2.286)
          (name "A" (effects (font (size 1.27 1.27))))
          (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at  3.81 0 180) (length 2.286)
          (name "K" (effects (font (size 1.27 1.27))))
          (number "2" (effects (font (size 1.27 1.27)))))))

    ;── N-ch MOSFET  (G pin1 left, D pin2 TOP y=-7.62, S pin3 BOTTOM y=+7.62)
    (symbol "Cust:NMOS" (pin_names (offset 1.016)) (in_bom yes) (on_board yes)
      (symbol "NMOS_0_1"
        ; gate insulation bar
        (polyline (pts (xy -3.048  1.905) (xy -3.048 -1.905))
          (stroke (width 0.508)) (fill (type none)))
        ; channel body
        (polyline (pts (xy -2.032  3.048) (xy -2.032 -3.048))
          (stroke (width 0.254)) (fill (type none)))
        ; stubs drain/mid/source
        (polyline (pts (xy -2.032 -1.905) (xy 0 -1.905))
          (stroke (width 0)) (fill (type none)))
        (polyline (pts (xy -2.032  0)     (xy 0  0))
          (stroke (width 0)) (fill (type none)))
        (polyline (pts (xy -2.032  1.905) (xy 0  1.905))
          (stroke (width 0)) (fill (type none)))
        ; N-ch arrow (middle stub)
        (polyline (pts (xy -3.048 -0.508) (xy -2.032 0) (xy -3.048 0.508) (xy -3.048 -0.508))
          (stroke (width 0)) (fill (type outline)))
        ; drain lead (upward, negative y)
        (polyline (pts (xy 0 -1.905) (xy 0 -5.08))
          (stroke (width 0)) (fill (type none)))
        ; source lead (downward, positive y)
        (polyline (pts (xy 0  1.905) (xy 0  5.08))
          (stroke (width 0)) (fill (type none)))
        ; gate lead
        (polyline (pts (xy -5.08 0) (xy -3.048 0))
          (stroke (width 0)) (fill (type none)))
        ; body diode (anode=source, cathode=drain) right side
        (polyline (pts (xy 1.016  1.905) (xy 1.016 -1.905))
          (stroke (width 0)) (fill (type none)))
        (polyline
          (pts (xy 0.508 1.905) (xy 2.032 1.905) (xy 1.27 -1.905) (xy 0.508 1.905))
          (stroke (width 0.254)) (fill (type background)))
        (polyline (pts (xy 0.508 -1.905) (xy 2.032 -1.905))
          (stroke (width 0.508)) (fill (type none)))
        (polyline (pts (xy 0 -1.905) (xy 1.016 -1.905))
          (stroke (width 0)) (fill (type none)))
        (polyline (pts (xy 0  1.905) (xy 1.016  1.905))
          (stroke (width 0)) (fill (type none))))
      (symbol "NMOS_1_1"
        ; G: left side
        (pin input   line (at -7.62  0     0) (length 2.54)
          (name "G" (effects (font (size 1.27 1.27))))
          (number "1" (effects (font (size 1.27 1.27)))))
        ; D: TOP  (at (0, -7.62), direction 270=downward into symbol)
        (pin passive line (at  0    -7.62 270) (length 2.54)
          (name "D" (effects (font (size 1.27 1.27))))
          (number "2" (effects (font (size 1.27 1.27)))))
        ; S: BOTTOM (at (0, +7.62), direction 90=upward into symbol)
        (pin passive line (at  0     7.62  90) (length 2.54)
          (name "S" (effects (font (size 1.27 1.27))))
          (number "3" (effects (font (size 1.27 1.27)))))))

    ;── IR2110 Gate Driver IC ───────────────────────────────────────────────
    (symbol "Cust:IR2110" (pin_names (offset 1.016)) (in_bom yes) (on_board yes)
      (symbol "IR2110_0_1"
        (rectangle (start -7.62 -12.7) (end 7.62 10.16)
          (stroke (width 0.254)) (fill (type background))))
      (symbol "IR2110_1_1"
        ; LEFT side – inputs (pin points LEFT = angle 180)
        (pin input    line (at -10.16  8.89 0) (length 2.54)
          (name "HIN" (effects (font (size 1.27 1.27))))
          (number "1"  (effects (font (size 1.27 1.27)))))
        (pin input    line (at -10.16  6.35 0) (length 2.54)
          (name "LIN" (effects (font (size 1.27 1.27))))
          (number "2"  (effects (font (size 1.27 1.27)))))
        (pin input    line (at -10.16  3.81 0) (length 2.54)
          (name "SD"  (effects (font (size 1.27 1.27))))
          (number "3"  (effects (font (size 1.27 1.27)))))
        (pin power_in line (at -10.16  1.27 0) (length 2.54)
          (name "VDD" (effects (font (size 1.27 1.27))))
          (number "4"  (effects (font (size 1.27 1.27)))))
        ; RIGHT side – power + outputs (pin points RIGHT = angle 0)
        (pin power_in line (at  10.16  8.89 180) (length 2.54)
          (name "VCC" (effects (font (size 1.27 1.27))))
          (number "5"  (effects (font (size 1.27 1.27)))))
        (pin power_in line (at  10.16  6.35 180) (length 2.54)
          (name "VB"  (effects (font (size 1.27 1.27))))
          (number "6"  (effects (font (size 1.27 1.27)))))
        (pin output   line (at  10.16  3.81 180) (length 2.54)
          (name "HO"  (effects (font (size 1.27 1.27))))
          (number "7"  (effects (font (size 1.27 1.27)))))
        (pin bidirectional line (at 10.16  1.27 180) (length 2.54)
          (name "VS"  (effects (font (size 1.27 1.27))))
          (number "8"  (effects (font (size 1.27 1.27)))))
        (pin output   line (at  10.16 -1.27 180) (length 2.54)
          (name "LO"  (effects (font (size 1.27 1.27))))
          (number "9"  (effects (font (size 1.27 1.27)))))
        (pin power_in line (at  10.16 -3.81 180) (length 2.54)
          (name "COM" (effects (font (size 1.27 1.27))))
          (number "10" (effects (font (size 1.27 1.27)))))))

    ;── Power symbols ────────────────────────────────────────────────────────
    (symbol "power:GND" (power) (pin_names (offset 0)) (in_bom yes) (on_board yes)
      (symbol "GND_0_1"
        (polyline (pts (xy 0 0) (xy 0 -1.27))
          (stroke (width 0)) (fill (type none)))
        (polyline (pts (xy -1.27 -1.27) (xy 1.27 -1.27))
          (stroke (width 0)) (fill (type none)))
        (polyline (pts (xy -0.762 -1.778) (xy 0.762 -1.778))
          (stroke (width 0)) (fill (type none)))
        (polyline (pts (xy -0.254 -2.286) (xy 0.254 -2.286))
          (stroke (width 0)) (fill (type none))))
      (symbol "GND_1_1"
        (pin power_in line (at 0 0 270) (length 0)
          (name "GND" (effects (font (size 1.27 1.27))))
          (number "1"  (effects (font (size 1.27 1.27)))))))

    (symbol "power:VCC" (power) (pin_names (offset 0)) (in_bom yes) (on_board yes)
      (symbol "VCC_0_1"
        (polyline (pts (xy -0.762 0.508) (xy 0 1.778) (xy 0.762 0.508))
          (stroke (width 0)) (fill (type none)))
        (polyline (pts (xy 0 0) (xy 0 1.778))
          (stroke (width 0)) (fill (type none))))
      (symbol "VCC_1_1"
        (pin power_in line (at 0 0 270) (length 0)
          (name "VCC" (effects (font (size 1.27 1.27))))
          (number "1"  (effects (font (size 1.27 1.27)))))))

    (symbol "power:+48V" (power) (pin_names (offset 0)) (in_bom yes) (on_board yes)
      (symbol "+48V_0_1"
        (polyline (pts (xy -0.762 0.508) (xy 0 1.778) (xy 0.762 0.508))
          (stroke (width 0)) (fill (type none)))
        (polyline (pts (xy 0 0) (xy 0 1.778))
          (stroke (width 0)) (fill (type none))))
      (symbol "+48V_1_1"
        (pin power_in line (at 0 0 270) (length 0)
          (name "+48V" (effects (font (size 1.27 1.27))))
          (number "1"   (effects (font (size 1.27 1.27)))))))
  )
"""

# ═══════════════════════════════════════════════════════════════════
# Component positions  (all in mm, 2.54 grid)
# ═══════════════════════════════════════════════════════════════════

# IR2110
ICX, ICY = 0.0, 0.0
# pin absolute positions (component at 0,0 no rotation):
#   left pins:  IC_LEFT_X = ICX - 10.16
#   right pins: IC_RIGHT_X = ICX + 10.16
IC_LX = ICX - 10.16
IC_RX = ICX + 10.16
# y offsets for each pin (positive y = down)
IC_PINS = {
    "HIN": ICY + 8.89,  "LIN": ICY + 6.35,
    "SD" : ICY + 3.81,  "VDD": ICY + 1.27,
    "VCC": ICY + 8.89,  "VB" : ICY + 6.35,
    "HO" : ICY + 3.81,  "VS" : ICY + 1.27,
    "LO" : ICY - 1.27,  "COM": ICY - 3.81,
}

# MOSFETs
Q1X, Q1Y = 60.0, -35.0   # hi-side
Q2X, Q2Y = 60.0,  35.0   # lo-side

# Pin offsets for NMOS  (no rotation, angle=0)
# G: dx=-7.62 dy=0  D: dx=0 dy=-7.62 (TOP)  S: dx=0 dy=+7.62 (BOTTOM)
def q_gate(cx,cy):   return (cx - 7.62, cy)
def q_drain(cx,cy):  return (cx, cy - 7.62)   # TOP
def q_source(cx,cy): return (cx, cy + 7.62)   # BOTTOM

Q1G = q_gate  (Q1X, Q1Y)
Q1D = q_drain (Q1X, Q1Y)
Q1S = q_source(Q1X, Q1Y)

Q2G = q_gate  (Q2X, Q2Y)
Q2D = q_drain (Q2X, Q2Y)
Q2S = q_source(Q2X, Q2Y)

# VS (switching node) – midpoint between Q1S and Q2D
VS_X = Q1X
VS_Y = 0.0     # between Q1S (y=-27.38) and Q2D (y=27.38)

# Gate resistors (placed horizontally = rotated 90°)
# When rotated 90° CCW: pin1 moves to RIGHT, pin2 to LEFT
# Cust:R pin1 at (0,-3.81) unrotated → after 90°CCW: (3.81,0) → RIGHT
# Cust:R pin2 at (0,+3.81) unrotated → after 90°CCW: (-3.81,0) → LEFT
R1X, R1Y = 37.0, Q1Y    # gate resistor hi-side
R2X, R2Y = 37.0, Q2Y    # gate resistor lo-side
R1P1 = (R1X + 3.81, R1Y)   # right → Q1 gate side
R1P2 = (R1X - 3.81, R1Y)   # left  → IC HO side
R2P1 = (R2X + 3.81, R2Y)
R2P2 = (R2X - 3.81, R2Y)

# Rgs resistors (vertical, between gate and source net)
# pin1 top (0,-3.81)→(rx, ry-3.81), pin2 bottom (0,+3.81)→(rx,ry+3.81)
R3X, R3Y = Q1X - 10.0, Q1Y + 3.81   # hi-side Rgs, between gate-y and source-y
R4X, R4Y = Q2X - 10.0, Q2Y - 3.81   # lo-side Rgs
R3P1 = (R3X, R3Y - 3.81)   # top → gate net
R3P2 = (R3X, R3Y + 3.81)   # bottom → VS net
R4P1 = (R4X, R4Y - 3.81)
R4P2 = (R4X, R4Y + 3.81)

# Bootstrap diode (horizontal, no rotation)
# pin1=Anode at (dx=-3.81,dy=0), pin2=Cathode at (dx=+3.81,dy=0)
D1X, D1Y = 25.0, -52.0
D1A = (D1X - 3.81, D1Y)
D1K = (D1X + 3.81, D1Y)    # VB node

# Bootstrap cap (vertical, between VB and VS)
C1X, C1Y = 45.0, -26.0
C1P1 = (C1X, C1Y - 2.032)  # top → VB
C1P2 = (C1X, C1Y + 2.032)  # bottom → VS

# VCC decoupling cap (vertical, near IC)
C2X, C2Y = IC_RX + 10, ICY + 8.89
C2P1 = (C2X, C2Y - 2.032)  # top → VCC
C2P2 = (C2X, C2Y + 2.032)  # bottom → GND

# ═══════════════════════════════════════════════════════════════════
# Build the schematic
# ═══════════════════════════════════════════════════════════════════
emit("(kicad_sch")
emit("  (version 20211123)")
emit("  (generator eeschema)")
emit()
emit('  (paper "A2")')
emit()
emit("  (title_block")
emit('    (title "Gate Driver – Half-Bridge  |  48V/500W Motor Inverter")')
emit('    (date "2026-05-24") (rev "1.0")')
emit('    (company "Project: 01_Inverter_500W")')
emit('    (comment 1 "MOSFET: IRFB4110 (100V/120A)  |  Driver: IR2110/EG3013")')
emit('    (comment 2 "R_gate=10ohm  |  C_boot=100nF/100V  |  D_boot=UF4007  |  Rgs=10kohm")')
emit("  )")
emit()
emit(LIB)

# ── Place components ──────────────────────────────────────────────────────────

# U1 – IR2110
place_comp("Cust:IR2110","U1","IR2110/EG3013", ICX,ICY, 0, 10)

# Q1 hi-side MOSFET
place_comp("Cust:NMOS","Q1","IRFB4110", Q1X,Q1Y, 0, 3)

# Q2 lo-side MOSFET
place_comp("Cust:NMOS","Q2","IRFB4110", Q2X,Q2Y, 0, 3)

# R1 gate hi (rotated 90 CCW)
place_comp("Cust:R","R1","10", R1X,R1Y, 90, 2)

# R2 gate lo (rotated 90 CCW)
place_comp("Cust:R","R2","10", R2X,R2Y, 90, 2)

# R3 Rgs hi (vertical, no rotation)
place_comp("Cust:R","R3","10k", R3X,R3Y, 0, 2)

# R4 Rgs lo (vertical, no rotation)
place_comp("Cust:R","R4","10k", R4X,R4Y, 0, 2)

# D1 bootstrap diode (horizontal)
place_comp("Cust:D","D1","UF4007", D1X,D1Y, 0, 2)

# C1 bootstrap cap (vertical)
place_comp("Cust:C","C1","100nF/100V", C1X,C1Y, 0, 2)

# C2 VCC decoupling (vertical)
place_comp("Cust:C","C2","100nF/25V", C2X,C2Y, 0, 2)

# ── Power symbols ─────────────────────────────────────────────────────────────

# +48V at Q1 drain (top)
pwr_sym("power:+48V","#PWR01", Q1D[0], Q1D[1]-3.81, 0)

# GND at Q2 source (bottom)
pwr_sym("power:GND","#PWR02", Q2S[0], Q2S[1]+1.27, 0)

# GND at COM
pwr_sym("power:GND","#PWR03", IC_RX+3, IC_PINS["COM"]+1.27, 0)

# VCC at IC VCC pin
pwr_sym("power:VCC","#PWR04", IC_RX+3, IC_PINS["VCC"]-3.81, 0)

# VCC at D1 anode
pwr_sym("power:VCC","#PWR05", D1A[0]-1.27, D1A[1]-3.81, 0)

# VCC at C2 top
pwr_sym("power:VCC","#PWR06", C2P1[0], C2P1[1]-3.81, 0)

# GND at C2 bottom
pwr_sym("power:GND","#PWR07", C2P2[0], C2P2[1]+1.27, 0)

# GND at R4 bottom (Rgs lo)
pwr_sym("power:GND","#PWR08", R4P2[0], R4P2[1]+1.27, 0)

# ── Wires ─────────────────────────────────────────────────────────────────────

# +48V → Q1 drain
wire(Q1D[0], Q1D[1]-3.81+1.27, Q1D[0], Q1D[1])

# Q1 source → VS node
wire(Q1S[0], Q1S[1], VS_X, VS_Y)
junc(VS_X, VS_Y) if abs(Q1S[1]-VS_Y) > 0.1 else None

# Q2 drain → VS node
wire(Q2D[0], Q2D[1], VS_X, VS_Y)

# Q2 source → GND
wire(Q2S[0], Q2S[1], Q2S[0], Q2S[1]+1.27-0.01)

# VS → Phase output label
wire(VS_X, VS_Y, VS_X+15, VS_Y)
net_label(VS_X+15, VS_Y, "Phase_Out_U", angle=0)

# IC VS pin → VS node (route: right then down)
wire(IC_RX, IC_PINS["VS"], VS_X-10, IC_PINS["VS"])
wire(VS_X-10, IC_PINS["VS"], VS_X-10, VS_Y)
wire(VS_X-10, VS_Y, VS_X, VS_Y)
junc(VS_X, VS_Y)

# IC COM → GND
wire(IC_RX, IC_PINS["COM"], IC_RX+3, IC_PINS["COM"])

# IC VCC → VCC power symbol
wire(IC_RX, IC_PINS["VCC"], IC_RX+3, IC_PINS["VCC"])

# C2 top → VCC, C2 bottom → GND (just connect to power symbols above)

# ── Bootstrap circuit ──
# D1 anode: VCC power symbol directly above it (placed already)
# D1 cathode = VB → net label + IC VB pin + C1 top
VB_NL_X = D1K[0] + 2.54
wire(D1K[0], D1K[1], VB_NL_X, D1K[1])
net_label(VB_NL_X, D1K[1], "VB")

# IC VB pin → VB net label
wire(IC_RX, IC_PINS["VB"], IC_RX+5, IC_PINS["VB"])
net_label(IC_RX+5, IC_PINS["VB"], "VB")

# C1 top → VB
wire(C1P1[0], C1P1[1], C1P1[0], C1P1[1]-2.54)
net_label(C1P1[0], C1P1[1]-2.54, "VB")

# C1 bottom → VS
wire(C1P2[0], C1P2[1], C1P2[0]+2.54, C1P2[1])
net_label(C1P2[0]+2.54, C1P2[1], "Phase_Out_U")

# ── Gate drive wires ──
# IC HO → route down to R1 level → R1 pin2
wire(IC_RX, IC_PINS["HO"], IC_RX+5, IC_PINS["HO"])
wire(IC_RX+5, IC_PINS["HO"], IC_RX+5, R1Y)
wire(IC_RX+5, R1Y, R1P2[0], R1P2[1])

# R1 pin1 → Q1 gate
wire(R1P1[0], R1P1[1], Q1G[0], Q1G[1])

# R3 (Rgs hi): pin1(top) → GATE_HI net, pin2(bottom) → VS net
wire(R3P1[0], R3P1[1], R3P1[0]+2.54, R3P1[1])
net_label(R3P1[0]+2.54, R3P1[1], "GATE_HI")

wire(Q1G[0], Q1G[1], Q1G[0]-2.54, Q1G[1])
net_label(Q1G[0]-2.54, Q1G[1], "GATE_HI")

wire(R1P1[0]+0.5, R1P1[1], R1P1[0]+3, R1P1[1])
net_label(R1P1[0]+3, R1P1[1], "GATE_HI")

wire(R3P2[0], R3P2[1], R3P2[0]+2.54, R3P2[1])
net_label(R3P2[0]+2.54, R3P2[1], "Phase_Out_U")

# IC LO → route up to R2 level → R2 pin2
wire(IC_RX, IC_PINS["LO"], IC_RX+8, IC_PINS["LO"])
wire(IC_RX+8, IC_PINS["LO"], IC_RX+8, R2Y)
wire(IC_RX+8, R2Y, R2P2[0], R2P2[1])

# R2 pin1 → Q2 gate
wire(R2P1[0], R2P1[1], Q2G[0], Q2G[1])

# R4 (Rgs lo): pin1(top) → GATE_LO net, pin2(bottom) → GND power
wire(R4P1[0], R4P1[1], R4P1[0]+2.54, R4P1[1])
net_label(R4P1[0]+2.54, R4P1[1], "GATE_LO")

wire(Q2G[0], Q2G[1], Q2G[0]-2.54, Q2G[1])
net_label(Q2G[0]-2.54, Q2G[1], "GATE_LO")

wire(R2P1[0]+0.5, R2P1[1], R2P1[0]+3, R2P1[1])
net_label(R2P1[0]+3, R2P1[1], "GATE_LO")

# ── MCU input net labels at IC left pins ──
for pname, lbl in [("HIN","MCU_HIN"),("LIN","MCU_LIN"),
                    ("SD","MCU_SD"),  ("VDD","VDD_5V")]:
    wire(IC_LX, IC_PINS[pname], IC_LX-3, IC_PINS[pname])
    net_label(IC_LX-3, IC_PINS[pname], lbl, angle=180)

# ── Annotations ──────────────────────────────────────────────────────────────
note(-25, 55, "!! DEAD TIME >= 500ns minimum !!", size=1.5)
note(-25, 58, "HIN and LIN must NEVER be HIGH at the same time", size=1.2)
note(-25, 61, "Prevents shoot-through: Q1+Q2 both ON = +48V shorted to GND!", size=1.2)

# ── Sheet instances ───────────────────────────────────────────────────────────
emit()
emit("  (sheet_instances")
emit('    (path "/" (page "1"))')
emit("  )")
emit(")")

# ── Save ──────────────────────────────────────────────────────────────────────
out = os.path.join(OUT, "gate_driver_halfbridge.kicad_sch")
with open(out, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))

print(f"Saved : {out}")
print(f"Lines : {len(lines)}")
print(f"Wires : {sum(1 for l in lines if '(wire ' in l)}")
print(f"Labels: {sum(1 for l in lines if '(label ' in l)}")
