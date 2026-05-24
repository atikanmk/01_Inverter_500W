# -*- coding: utf-8 -*-
"""
Generate EasyEDA Standard Edition JSON schematic
48V/500W Motor Inverter – Gate Driver (one half-bridge)
Import: EasyEDA > File > Import > EasyEDA Source
"""
import json, os

OUT = os.path.dirname(os.path.abspath(__file__))

# ── ID counter ────────────────────────────────────────────────────────────────
_id = [0]
def uid():
    _id[0] += 1
    return f"gge{_id[0]}"

shapes = []

# ── Primitive helpers ──────────────────────────────────────────────────────────
def W(x1,y1,x2,y2, col="#000000", w=1):
    """Electrical wire"""
    shapes.append(f"W~{x1} {y1} {x2} {y2}~{col}~{w}~0~none~{uid()}")

def L(x1,y1,x2,y2, col="#000000", w=1):
    """Graphical line"""
    shapes.append(f"L~{x1}~{y1}~{x2}~{y2}~{col}~{w}~0~none~{uid()}")

def R(x,y,rw,rh, stroke="#000000", fill="#ffffff", sw=1):
    """Rectangle, top-left at (x,y)"""
    shapes.append(f"R~{x}~{y}~0~0~{rw}~{rh}~{stroke}~{sw}~0~{fill}~{uid()}")

def PL(pts, stroke="#000000", fill="none", w=1):
    """Polyline/polygon"""
    s = " ".join(f"{p[0]} {p[1]}" for p in pts)
    shapes.append(f"PL~{s}~{stroke}~{w}~0~{fill}~{uid()}")

def J(x,y):
    """Junction dot"""
    shapes.append(f"J~{x}~{y}~3~#000000~{uid()}")

def T(x,y,txt, rot=0, col="#000000", sz="7pt", anchor="middle"):
    """Schematic text annotation"""
    shapes.append(f"T~{rot}~{x}~{y}~{anchor}~1~{col}~Arial~{sz}~normal~normal~{txt}~{uid()}")

def NL(x,y,name, rot=0):
    """Net label"""
    shapes.append(f"N~{x}~{y}~{rot}~5~{name}~#0000FF~Arial~normal~normal~p~{uid()}")

# ── Component symbols ──────────────────────────────────────────────────────────

def draw_ic(x,y,w,h, name, sub,
            right_pins, left_pins,
            box_stroke="#006400", box_fill="#E2EFDA"):
    """IC block. right/left_pins = [(name_str, y_offset_from_box_top), ...]
    Returns dict: pin_name -> (x_conn, y_conn)"""
    R(x,y,w,h, stroke=box_stroke, fill=box_fill, sw=2)
    T(x+w//2, y+22, name,  col=box_stroke, sz="8pt")
    T(x+w//2, y+42, sub,   col="#595959",  sz="6pt")
    stub = 30
    coords = {}
    for pname, yoff in right_pins:
        px, py = x+w, y+yoff
        L(px, py, px+stub, py, col="#0047FF")
        T(px-5, py-10, pname, anchor="end",   sz="6pt", col="#0047FF")
        T(px+stub+4, py, pname, anchor="start", sz="6pt", col="#0047FF")
        coords[pname] = (px+stub, py)
    for pname, yoff in left_pins:
        px, py = x, y+yoff
        L(px-stub, py, px, py, col="#0047FF")
        T(px+5, py-10, pname, anchor="start", sz="6pt", col="#0047FF")
        T(px-stub-4, py, pname, anchor="end", sz="6pt", col="#0047FF")
        coords[pname] = (px-stub, py)
    return coords


def draw_nmos(cx,cy, ref="Q", val="IRFB4110", lead=70):
    """N-ch MOSFET symbol.
    Returns gate(x,y), drain(x,y), source(x,y)"""
    gbx = cx-20   # gate bar x
    chx = cx-10   # channel x

    # gate insulation bar
    L(gbx, cy-30, gbx, cy+30, w=2)
    # gate lead
    gate = (gbx-30, cy)
    L(gate[0], cy, gbx, cy)
    # channel body
    L(chx, cy-25, chx, cy+25, w=2)
    # stubs
    L(gbx+2, cy-20, chx, cy-20)
    L(gbx+2, cy,    chx, cy)
    L(gbx+2, cy+20, chx, cy+20)
    # N-ch arrow on centre stub
    PL([(gbx+2,cy-4),(gbx+2,cy+4),(gbx+14,cy)], fill="#000000")

    # drain lead
    drain = (cx, cy-lead)
    L(chx, cy-20, cx, cy-20)
    W(cx, cy-20, cx, cy-lead)
    # source lead
    source = (cx, cy+lead)
    L(chx, cy+20, cx, cy+20)
    W(cx, cy+20, cx, cy+lead)

    # body diode (right side)
    ddx = cx+16
    PL([(ddx,cy-20),(ddx,cy+20),(ddx+18,cy)],
       stroke="#808000", fill="#FFFFA0", w=1)
    L(ddx+18, cy-20, ddx+18, cy+20, col="#808000")
    L(cx, cy-20, ddx, cy-20, col="#808080")
    L(cx, cy+20, ddx, cy+20, col="#808080")

    # labels
    T(gate[0]-4, cy-18, ref, anchor="end", sz="7pt", col="#C00000")
    T(gate[0]-4, cy+6,  val, anchor="end", sz="6pt", col="#595959")
    return gate, drain, source


def draw_res_h(x1,y1,x2,y2, ref="R", val=""):
    """Horizontal resistor zigzag"""
    dx = x2-x1; n=8; seg=dx/n
    pts=[(x1,y1)]
    for i in range(1,n):
        pts.append((x1+i*seg, y1+(-10 if i%2==0 else 10)))
    pts.append((x2,y2))
    PL(pts)
    mx=(x1+x2)//2
    T(mx, y1-16, ref, sz="6pt")
    T(mx, y1+20, val, sz="6pt", col="#595959")


def draw_cap_v(cx,y1,y2, ref="C", val=""):
    """Vertical capacitor"""
    my=(y1+y2)//2; arm=22
    W(cx,y1, cx,my-9)
    L(cx-arm, my-9, cx+arm, my-9, w=2)
    L(cx-arm, my+9, cx+arm, my+9, w=2)
    W(cx,my+9, cx,y2)
    T(cx+arm+5, my-9, ref, anchor="start", sz="6pt")
    T(cx+arm+5, my+9, val, anchor="start", sz="6pt", col="#595959")


def draw_diode_h(x1,y,x2,_, ref="D", val=""):
    """Horizontal diode: anode left, cathode right"""
    mx=(x1+x2)//2; h=15
    W(x1,y, mx-h,y)
    PL([(mx-h,y-h),(mx-h,y+h),(mx+h,y)], stroke="#808000", fill="#FFFFA0")
    L(mx+h,y-h, mx+h,y+h, col="#808000", w=2)
    W(mx+h,y, x2,y)
    T(mx, y-23, ref, sz="6pt")
    T(mx, y+23, val, sz="6pt", col="#595959")


def draw_vcc(x,y, name="+15V", col="#C55A11"):
    """VCC power symbol (bar on top)"""
    W(x,y, x,y-20, col=col)
    L(x-15,y-20, x+15,y-20, col=col, w=2)
    T(x, y-33, name, col=col, sz="7pt")


def draw_gnd(x,y):
    """GND power symbol (three descending bars)"""
    W(x,y, x,y+22)
    L(x-16,y+22, x+16,y+22, w=2)
    L(x-10,y+32, x+10,y+32, w=2)
    L(x-4, y+42, x+4, y+42, w=2)
    T(x, y+52, "GND", sz="7pt")


def draw_bus_rail(x1,y,x2, label, col, lbl_side="left"):
    """Horizontal power bus rail"""
    W(x1,y,x2,y, col=col, w=3)
    ax = x1-10 if lbl_side=="left" else x2+10
    anc = "end" if lbl_side=="left" else "start"
    T(ax, y, label, anchor=anc, sz="9pt", col=col)


# ══════════════════════════════════════════════════════════════════════════════
# Build schematic: Gate Driver – One Half-Bridge
# ══════════════════════════════════════════════════════════════════════════════
#
# Layout (pixels, grid=10):
#
#   +48V bus ─────────────────────────────────────  y=100
#   VCC(+15V) ──── D_boot ── VB ──C_boot── VS      y=230
#                                           │
#                [IR2110]  HO─[Rg]─ Q_hi gate    y=350-900
#                          LO─[Rg]─ Q_lo gate
#                           VS ────────────────   y=630 (switching node)
#   GND bus ──────────────────────────────────────  y=1100

# Canvas bounds
CW, CH = 1800, 1250

# Key y-positions
Y_48V   = 100
Y_VCC   = 230
Y_GND   = 1100
Y_VS    = 630   # switching node

# IR2110 IC box
IC_X, IC_Y = 160, 340
IC_W, IC_H = 230, 560

ic_right = [("VCC", 65),("VB", 145),("HO", 225),("VS", 310),("LO", 410),("COM",510)]
ic_left  = [("HIN",110),("LIN",230),("SD", 360),("VDD",470)]

pins = draw_ic(IC_X, IC_Y, IC_W, IC_H,
               "IR2110 / EG3013","Half-Bridge Gate Driver",
               ic_right, ic_left)

# ── +48V and GND bus rails ────────────────────────────────────────────────────
draw_bus_rail(460, Y_48V, 1600, "+48V  (DC Bus)", "#C00000", "left")
draw_bus_rail(460, Y_GND, 1600, "GND",             "#404040", "left")

# ── VCC rail ──────────────────────────────────────────────────────────────────
W(80,Y_VCC, 520,Y_VCC, col="#C55A11", w=2)
T(65,Y_VCC, "+15V (VCC)", anchor="end", sz="8pt", col="#C55A11")

# C_vcc decoupling
W(200,Y_VCC, 200,310)
draw_cap_v(200, 310, 490, ref="C_vcc", val="100nF")
W(200,490, 200,Y_GND, col="#404040")
draw_gnd(200, Y_GND)

# VCC → IC VCC pin
pVCC = pins["VCC"]
W(IC_X+IC_W, pVCC[1], 520, pVCC[1])
W(520, pVCC[1], 520, Y_VCC)
J(520, Y_VCC)

# ── Bootstrap circuit ─────────────────────────────────────────────────────────
# Anode of D_boot from VCC rail at x=540
BOOT_AX = 540   # diode anode x
BOOT_CX = 700   # diode cathode / VB node x

W(BOOT_AX, Y_VCC, BOOT_AX, Y_VCC-10)   # down stub from VCC
draw_diode_h(BOOT_AX, Y_VCC-30, BOOT_CX, Y_VCC-30,
             ref="D_boot", val="UF4007")
# VB node
VB_X, VB_Y = BOOT_CX, Y_VCC-30
T(VB_X+5, VB_Y-18, "VB", anchor="start", sz="7pt", col="#C55A11")

# C_boot: VB → VS
W(VB_X, VB_Y, VB_X, VB_Y+20)
draw_cap_v(VB_X, VB_Y+20, Y_VS-10,
           ref="C_boot", val="100nF/100V")
W(VB_X, Y_VS-10, VB_X, Y_VS)

# VB IC pin connection
pVB = pins["VB"]
W(IC_X+IC_W, pVB[1], VB_X, pVB[1])
W(VB_X, pVB[1], VB_X, VB_Y)
J(VB_X, VB_Y)

# ── Q_hi (High-side MOSFET) ───────────────────────────────────────────────────
QH_CX, QH_CY = 1020, 360
g_hi, d_hi, s_hi = draw_nmos(QH_CX, QH_CY,
                               ref="Q_hi", val="IRFB4110", lead=75)
# Drain → +48V bus
W(d_hi[0], d_hi[1], d_hi[0], Y_48V)
J(d_hi[0], Y_48V)

# Source → VS node wire
W(s_hi[0], s_hi[1], s_hi[0], Y_VS)

# VS node connects Q_hi source and Q_lo drain
VS_X = s_hi[0]   # = QH_CX = 1020
T(VS_X+40, Y_VS, "VS (Phase Out)", anchor="start", sz="7pt", col="#7030A0")
J(VS_X, Y_VS)

# VB-Cboot bottom → VS x
W(VB_X, Y_VS, VS_X, Y_VS)
J(VS_X, Y_VS)

# VS IC pin
pVS = pins["VS"]
W(IC_X+IC_W, pVS[1], 750, pVS[1])
W(750, pVS[1], 750, Y_VS)
W(750, Y_VS, VS_X, Y_VS)
J(750, Y_VS)

# HO → Rg_hi → Q_hi gate
pHO = pins["HO"]
ROUTE_HI_Y = QH_CY         # same height as Q_hi gate
W(pHO[0], pHO[1], pHO[0]+30, pHO[1])
W(pHO[0]+30, pHO[1], pHO[0]+30, ROUTE_HI_Y)
W(pHO[0]+30, ROUTE_HI_Y, 820, ROUTE_HI_Y)
draw_res_h(820, ROUTE_HI_Y, 960, ROUTE_HI_Y, ref="R_g", val="10Ω")
W(960, ROUTE_HI_Y, g_hi[0], ROUTE_HI_Y)
# gate lead connects
W(g_hi[0], g_hi[1], g_hi[0], ROUTE_HI_Y)

# R_gs_hi: gate → source pull-down
RGS_HI_X = g_hi[0]-20
W(g_hi[0], g_hi[1], RGS_HI_X, g_hi[1])
W(RGS_HI_X, g_hi[1], RGS_HI_X, g_hi[1]+30)
draw_cap_v(RGS_HI_X, g_hi[1]+30, Y_VS-20, ref="R_gs", val="10kΩ")
# use cap-v shape as stand-in; draw resistor instead
# (overwrite with proper resistor)
shapes.pop(); shapes.pop(); shapes.pop(); shapes.pop(); shapes.pop()
draw_res_h(RGS_HI_X, g_hi[1]+30, RGS_HI_X, Y_VS-20, ref="R_gs", val="10kΩ")

# Actually R_gs is vertical – draw it as vertical zigzag workaround:
shapes.pop(); shapes.pop(); shapes.pop()  # remove last resistor attempt
# vertical resistor: just draw it as a box + label
R(RGS_HI_X-12, g_hi[1]+30, 24, Y_VS-20-g_hi[1]-30,
  stroke="#000000", fill="#FFFFF0")
T(RGS_HI_X-20, (g_hi[1]+30+Y_VS-20)//2, "R_gs",
  rot=90, anchor="middle", sz="6pt")
T(RGS_HI_X+16, (g_hi[1]+30+Y_VS-20)//2, "10kΩ",
  rot=90, anchor="middle", sz="6pt", col="#595959")
W(RGS_HI_X, g_hi[1]+30, RGS_HI_X, Y_VS-20)
W(RGS_HI_X, Y_VS-20, RGS_HI_X, Y_VS)
W(RGS_HI_X, Y_VS, VS_X, Y_VS)

# ── Q_lo (Low-side MOSFET) ────────────────────────────────────────────────────
QL_CX, QL_CY = 1020, 870
g_lo, d_lo, s_lo = draw_nmos(QL_CX, QL_CY,
                               ref="Q_lo", val="IRFB4110", lead=75)
# Drain → VS
W(d_lo[0], d_lo[1], d_lo[0], Y_VS)
J(d_lo[0], Y_VS)

# Source → GND bus
W(s_lo[0], s_lo[1], s_lo[0], Y_GND)
J(s_lo[0], Y_GND)

# LO → Rg_lo → Q_lo gate
pLO = pins["LO"]
ROUTE_LO_Y = QL_CY
W(pLO[0], pLO[1], pLO[0]+30, pLO[1])
W(pLO[0]+30, pLO[1], pLO[0]+30, ROUTE_LO_Y)
W(pLO[0]+30, ROUTE_LO_Y, 820, ROUTE_LO_Y)
draw_res_h(820, ROUTE_LO_Y, 960, ROUTE_LO_Y, ref="R_g", val="10Ω")
W(960, ROUTE_LO_Y, g_lo[0], ROUTE_LO_Y)
W(g_lo[0], g_lo[1], g_lo[0], ROUTE_LO_Y)

# R_gs_lo
RGS_LO_X = g_lo[0]-20
W(g_lo[0], g_lo[1], RGS_LO_X, g_lo[1])
W(RGS_LO_X, g_lo[1], RGS_LO_X, g_lo[1]+30)
R(RGS_LO_X-12, g_lo[1]+30, 24, s_lo[1]-g_lo[1]-30,
  stroke="#000000", fill="#FFFFF0")
T(RGS_LO_X-20, (g_lo[1]+30+s_lo[1])//2, "R_gs",
  rot=90, anchor="middle", sz="6pt")
T(RGS_LO_X+16, (g_lo[1]+30+s_lo[1])//2, "10kΩ",
  rot=90, anchor="middle", sz="6pt", col="#595959")
W(RGS_LO_X, g_lo[1]+30, RGS_LO_X, s_lo[1])
W(RGS_LO_X, s_lo[1], s_lo[0], s_lo[1])

# ── IC left-side inputs (MCU) ─────────────────────────────────────────────────
for pname, lbl in [("HIN","MCU HIN"),("LIN","MCU LIN"),
                   ("SD", "MCU SD"), ("VDD","+5V (VDD)")]:
    px, py = pins[pname]
    W(px, py, px-10, py)
    T(px-16, py, lbl, anchor="end", sz="7pt", col="#595959")

# IC COM → GND
pCOM = pins["COM"]
W(IC_X+IC_W, pCOM[1], 460, pCOM[1])
W(460, pCOM[1], 460, Y_GND)
J(460, Y_GND)

# ── Phase output arrow & label ─────────────────────────────────────────────────
W(VS_X, Y_VS, 1600, Y_VS, col="#7030A0", w=2)
# arrow tip
PL([(1595,Y_VS-10),(1610,Y_VS),(1595,Y_VS+10)],
   stroke="#7030A0", fill="#7030A0")
T(1620, Y_VS, "Phase Out", anchor="start", sz="9pt", col="#7030A0")
T(1620, Y_VS+18, "(to Motor)", anchor="start", sz="7pt", col="#595959")

# ── Dead-time warning box ─────────────────────────────────────────────────────
R(30,960, 340,150, stroke="#C00000", fill="#FCE4D6", sw=2)
T(200,978,  "!! DEAD TIME WARNING", col="#C00000", sz="7pt")
T(200,1003, "Dead Time >= 500 ns", col="#C00000", sz="6pt")
T(200,1023, "HIN & LIN NEVER HIGH together", col="#C00000", sz="6pt")
T(200,1043, "Set via MCU PWM timer register", col="#595959", sz="6pt")
T(200,1063, "Prevents shoot-through of Q_hi+Q_lo", col="#595959", sz="6pt")
T(200,1083, "which would short +48V to GND !", col="#C00000", sz="6pt")

# ── Title block ───────────────────────────────────────────────────────────────
R(0, CH-80, CW, 80, stroke="#1F4E79", fill="#D6E4F0", sw=2)
T(CW//2, CH-52,
  "Gate Driver – One Half-Bridge  |  48V / 500W Motor Inverter",
  sz="11pt", col="#1F4E79")
T(CW//2, CH-28,
  "MOSFET: IRFB4110 (100V/120A)  |  Driver: IR2110 / EG3013  "
  "|  R_gate=10ohm  |  C_boot=100nF/100V  |  D_boot=UF4007  |  R_gs=10kOhm",
  sz="7pt", col="#595959")
T(20, CH-52, "Rev 1.0", anchor="start", sz="7pt", col="#595959")
T(20, CH-28, "Project: 01_Inverter_500W", anchor="start", sz="7pt", col="#595959")

# ── Assemble EasyEDA JSON ─────────────────────────────────────────────────────
schematic = {
    "head": {
        "type": "schematic",
        "c_para": {},
        "hasIdFlag": True
    },
    "canvas": (
        f"CA~0~0~#ffffff~yes~#CCCCCC~10~{CW}~{CH}"
        f"~line~10~pixel~5~0~0~0"
    ),
    "shape": shapes,
    "BBox": {"x": 0, "y": 0, "width": CW, "height": CH},
    "netlist": []
}

out_path = os.path.join(OUT, "gate_driver_halfbridge_easyeda.json")
with open(out_path, "w", encoding="utf-8") as f:
    json.dump(schematic, f, ensure_ascii=False, indent=2)

print(f"Saved: {out_path}")
print(f"Total shapes: {len(shapes)}")
print()
print("Import steps:")
print("  EasyEDA Standard: File > Import > EasyEDA Source > select .json")
print("  EasyEDA Pro     : File > Import > EasyEDA (JSON) > select .json")
