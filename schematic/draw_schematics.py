# -*- coding: utf-8 -*-
"""
Generate two schematics for 48V/500W 3-phase inverter:
  1. 3phase_power_stage.svg/png  – full 3-phase H-bridge topology
  2. gate_driver_1phase.svg/png  – one half-bridge gate driver detail (IR2110)
"""
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import FancyArrow
import matplotlib.patheffects as pe
import numpy as np
import os

OUT = os.path.dirname(os.path.abspath(__file__))

# ── common style ──────────────────────────────────────────────────────────────
BUS_COLOR   = "#C00000"   # 48V rail
GND_COLOR   = "#404040"   # GND rail
WIRE_COLOR  = "#1F1F1F"
MOS_BODY    = "#2E75B6"
MOS_FILL    = "#DDEEFF"
IC_FILL     = "#E2EFDA"
IC_EDGE     = "#375623"
PHASE_COLOR = "#7030A0"
FONT        = "Arial"

LW_BUS  = 2.8
LW_WIRE = 1.6
LW_COMP = 1.6


def wire(ax, pts, color=WIRE_COLOR, lw=LW_WIRE, zorder=2):
    """Draw a polyline through list of (x,y) tuples."""
    xs = [p[0] for p in pts]
    ys = [p[1] for p in pts]
    ax.plot(xs, ys, color=color, lw=lw, solid_capstyle="round",
            solid_joinstyle="round", zorder=zorder)


def dot(ax, x, y, color=WIRE_COLOR, r=0.07):
    ax.add_patch(mpatches.Circle((x, y), r, color=color, zorder=5))


def label(ax, x, y, txt, ha="center", va="center", fs=8.5, bold=False,
          color="black", rotation=0):
    weight = "bold" if bold else "normal"
    ax.text(x, y, txt, ha=ha, va=va, fontsize=fs, fontname=FONT,
            fontweight=weight, color=color, rotation=rotation, zorder=10)


# ── MOSFET symbol (N-ch, drain=top, source=bottom) ───────────────────────────
# cx,cy = center of MOSFET body; returns (gate_xy, drain_xy, source_xy)
def draw_nmos(ax, cx, cy, h=1.0, lbl="", body_diode=True):
    bx = cx - 0.22 * h    # body / channel x
    gx = cx - 0.40 * h    # gate bar x
    dx = cx               # drain/source x (stub ends)

    top    = cy + 0.25 * h
    bot    = cy - 0.25 * h
    dtop   = cy + 0.60 * h    # drain terminal
    dbot   = cy - 0.60 * h    # source terminal
    gate_y = cy

    # Gate insulator bar
    ax.plot([gx, gx], [bot, top], color=MOS_BODY, lw=LW_COMP + 0.5, zorder=3)
    # Gate lead
    ax.plot([gx - 0.38 * h, gx], [gate_y, gate_y],
            color=WIRE_COLOR, lw=LW_WIRE, zorder=3)
    gate_xy = (gx - 0.38 * h, gate_y)

    # Channel stubs (drain, center, source)
    for sy in [top, cy, bot]:
        ax.plot([gx + 0.10 * h, dx], [sy, sy],
                color=MOS_BODY, lw=LW_COMP, zorder=3)

    # Drain lead (top)
    ax.plot([dx, dx], [top, dtop], color=WIRE_COLOR, lw=LW_WIRE, zorder=3)

    # Source lead with arrow (bottom) – arrow points toward body = N-ch
    ax.plot([dx, dx], [bot, dbot], color=WIRE_COLOR, lw=LW_WIRE, zorder=3)
    # small arrow on source stub
    ax.annotate("", xy=(gx + 0.14 * h, bot),
                xytext=(dx - 0.01, bot),
                arrowprops=dict(arrowstyle="-|>", color=MOS_BODY,
                                lw=LW_COMP, mutation_scale=10),
                zorder=4)

    # Body diode (anode=source, cathode=drain) — drawn to the right
    if body_diode:
        ddx = dx + 0.22 * h
        # diode body
        tri = mpatches.Polygon([[ddx, top], [ddx, bot],
                                 [ddx + 0.18 * h, cy]], closed=True,
                                facecolor="#FFF2CC", edgecolor=GND_COLOR,
                                lw=0.8, zorder=3)
        ax.add_patch(tri)
        ax.plot([ddx + 0.18 * h, ddx + 0.18 * h], [top, bot],
                color=GND_COLOR, lw=1.2, zorder=3)
        # connect to leads
        ax.plot([dx, ddx], [top, top], color=WIRE_COLOR, lw=0.9, zorder=3)
        ax.plot([dx, ddx], [bot, bot], color=WIRE_COLOR, lw=0.9, zorder=3)
        ax.plot([ddx + 0.18 * h, ddx + 0.18 * h], [top, dtop - 0.01],
                color=WIRE_COLOR, lw=0.9, zorder=3)
        ax.plot([ddx + 0.18 * h, ddx + 0.18 * h], [bot, dbot + 0.01],
                color=WIRE_COLOR, lw=0.9, zorder=3)

    if lbl:
        label(ax, gx - 0.6 * h, gate_y, lbl, ha="right", fs=8, bold=True)

    return gate_xy, (dx, dtop), (dx, dbot)


def draw_resistor(ax, x1, y1, x2, y2, lbl="", fs=7.5):
    """Vertical or horizontal resistor between two points."""
    dx, dy = x2 - x1, y2 - y1
    L = np.hypot(dx, dy)
    n = 6
    step = L / (n + 2)
    ts = np.linspace(0, 1, n * 2 + 2)
    xs = x1 + dx * ts
    ys = y1 + dy * ts
    # zigzag perpendicular
    perp = np.array([-dy, dx]) / L * 0.10
    pts_x = [x1]
    pts_y = [y1]
    for i in range(1, len(ts) - 1):
        sign = 1 if i % 2 == 1 else -1
        pts_x.append(xs[i] + sign * perp[0])
        pts_y.append(ys[i] + sign * perp[1])
    pts_x.append(x2)
    pts_y.append(y2)
    ax.plot(pts_x, pts_y, color=WIRE_COLOR, lw=LW_COMP, zorder=3)
    mx, my = (x1 + x2) / 2, (y1 + y2) / 2
    if lbl:
        off = perp * 3.5
        label(ax, mx + off[0], my + off[1], lbl, fs=fs)


def draw_capacitor(ax, cx, cy, horiz=False, lbl="", fs=7.5):
    gap = 0.08
    arm = 0.20
    if not horiz:  # vertical cap (leads top/bottom)
        ax.plot([cx - arm, cx + arm], [cy + gap, cy + gap],
                color=WIRE_COLOR, lw=LW_COMP + 0.5, zorder=3)
        ax.plot([cx - arm, cx + arm], [cy - gap, cy - gap],
                color=WIRE_COLOR, lw=LW_COMP + 0.5, zorder=3)
        if lbl:
            label(ax, cx + arm + 0.18, cy, lbl, ha="left", fs=fs)
        return (cx, cy + gap), (cx, cy - gap)
    else:          # horizontal cap (leads left/right)
        ax.plot([cx + gap, cx + gap], [cy - arm, cy + arm],
                color=WIRE_COLOR, lw=LW_COMP + 0.5, zorder=3)
        ax.plot([cx - gap, cx - gap], [cy - arm, cy + arm],
                color=WIRE_COLOR, lw=LW_COMP + 0.5, zorder=3)
        if lbl:
            label(ax, cx, cy + arm + 0.15, lbl, ha="center", fs=fs)
        return (cx + gap, cy), (cx - gap, cy)


def draw_diode(ax, ax_, ay, bx, by, lbl=""):
    """Draw diode from (ax_,ay) anode to (bx,by) cathode (vertical assumed)."""
    mx, my = (ax_ + bx) / 2, (ay + by) / 2
    h = 0.18
    tri = mpatches.Polygon([[mx, my + h], [mx, my - h], [mx + h, my]],
                            closed=True, facecolor="#FFF2CC",
                            edgecolor=WIRE_COLOR, lw=1.2, zorder=4)
    ax.add_patch(tri)
    ax.plot([mx + h, mx + h], [my - h, my + h],
            color=WIRE_COLOR, lw=1.5, zorder=4)
    ax.plot([ax_, mx], [ay, my + h], color=WIRE_COLOR, lw=LW_WIRE, zorder=3)
    ax.plot([mx, bx], [my - h, by], color=WIRE_COLOR, lw=LW_WIRE, zorder=3)
    if lbl:
        label(ax, mx - 0.22, my, lbl, ha="right", fs=7.5)


# ══════════════════════════════════════════════════════════════════════════════
# SCHEMATIC 1 – 3-Phase Power Stage
# ══════════════════════════════════════════════════════════════════════════════
def draw_power_stage():
    fig, ax = plt.subplots(figsize=(20, 11))
    ax.set_xlim(0, 20)
    ax.set_ylim(0, 11)
    ax.set_aspect("equal")
    ax.axis("off")
    fig.patch.set_facecolor("white")

    # Title
    ax.text(10, 10.5, "3-Phase Inverter Power Stage  |  48 V / 500 W",
            ha="center", va="center", fontsize=14, fontname=FONT,
            fontweight="bold", color="#1F4E79")
    ax.text(10, 10.1, "MOSFET: IRFB4110 (100V/120A)   Body Diode shown",
            ha="center", va="center", fontsize=9, fontname=FONT, color="#595959")

    # DC Bus rails
    bus_y, gnd_y = 9.2, 0.6
    ax.plot([1.0, 19.0], [bus_y, bus_y], color=BUS_COLOR, lw=LW_BUS, zorder=2)
    ax.plot([1.0, 19.0], [gnd_y, gnd_y], color=GND_COLOR, lw=LW_BUS, zorder=2)
    label(ax, 0.5, bus_y, "+48V", ha="right", fs=10, bold=True, color=BUS_COLOR)
    label(ax, 0.5, gnd_y, "GND",  ha="right", fs=10, bold=True, color=GND_COLOR)

    # Bulk capacitor
    wire(ax, [(1.5, bus_y), (1.5, bus_y - 0.3)], color=BUS_COLOR)
    wire(ax, [(1.5, gnd_y), (1.5, gnd_y + 0.3)], color=GND_COLOR)
    draw_capacitor(ax, 1.5, (bus_y + gnd_y) / 2, lbl="C_bulk\n470µF/63V")
    wire(ax, [(1.5, bus_y - 0.3), (1.5, (bus_y + gnd_y) / 2 + 0.08)],
         color=BUS_COLOR)
    wire(ax, [(1.5, gnd_y + 0.3), (1.5, (bus_y + gnd_y) / 2 - 0.08)],
         color=GND_COLOR)
    label(ax, 1.5, (bus_y + gnd_y) / 2 - 0.8, "Bulk\nCap", fs=7.5,
          color="#595959")

    # Phase columns: U=x4.5, V=x9, W=x13.5
    phases = [
        ("U", 4.5,  "Q1", "Q2"),
        ("V", 9.0,  "Q3", "Q4"),
        ("W", 13.5, "Q5", "Q6"),
    ]
    sw_nodes = {}   # phase → (x, y) of switching node

    h_mos = 1.0   # MOSFET scale
    hi_cy = 7.3   # hi-side center
    lo_cy = 2.5   # lo-side center

    for pname, px, qhi, qlo in phases:
        # Connect drain of Hi-MOSFET to 48V rail
        wire(ax, [(px, bus_y), (px, hi_cy + 0.60 * h_mos)], color=BUS_COLOR)
        # Hi-side MOSFET (drain up, source down)
        g_hi, d_hi, s_hi = draw_nmos(ax, px, hi_cy, h=h_mos, lbl=qhi)

        sw_y = (s_hi[1] + lo_cy + 0.60 * h_mos) / 2 + 0.1
        sw_nodes[pname] = (px, sw_y)

        # Switching node wire (Hi source ↔ Lo drain)
        wire(ax, [s_hi, (px, sw_y), (px, lo_cy + 0.60 * h_mos)])

        # Lo-side MOSFET
        g_lo, d_lo, s_lo = draw_nmos(ax, px, lo_cy, h=h_mos, lbl=qlo)
        # Connect source to GND
        wire(ax, [s_lo, (px, gnd_y)], color=GND_COLOR)

        # Gate drive stubs (just show gate terminal)
        gate_hi_x = g_hi[0]
        gate_lo_x = g_lo[0]
        wire(ax, [(gate_hi_x, g_hi[1]), (gate_hi_x - 0.4, g_hi[1])])
        wire(ax, [(gate_lo_x, g_lo[1]), (gate_lo_x - 0.4, g_lo[1])])
        ax.add_patch(mpatches.Circle(
            (gate_hi_x - 0.4, g_hi[1]), 0.05, color=WIRE_COLOR, zorder=5))
        ax.add_patch(mpatches.Circle(
            (gate_lo_x - 0.4, g_lo[1]), 0.05, color=WIRE_COLOR, zorder=5))
        label(ax, gate_hi_x - 0.55, g_hi[1], f"G_{qhi}", ha="right", fs=7,
              color="#7030A0")
        label(ax, gate_lo_x - 0.55, g_lo[1], f"G_{qlo}", ha="right", fs=7,
              color="#7030A0")

        # Switching node label
        dot(ax, px, sw_y)
        label(ax, px + 0.25, sw_y, pname, ha="left", fs=10, bold=True,
              color=PHASE_COLOR)

        # Phase column background
        rect = mpatches.FancyBboxPatch(
            (px - 0.9, 1.0), 1.8, 8.0,
            boxstyle="round,pad=0.1", linewidth=0.8,
            edgecolor="#BFBFBF", facecolor="#F7FBFF", zorder=0)
        ax.add_patch(rect)
        label(ax, px, 0.2, f"Phase {pname}", fs=8.5, bold=True,
              color=PHASE_COLOR)

    # Motor block
    mx0, mx1 = 16.0, 19.0
    my0, my1 = 3.5, 7.5
    motor_rect = mpatches.FancyBboxPatch(
        (mx0, my0), mx1 - mx0, my1 - my0,
        boxstyle="round,pad=0.15", linewidth=1.5,
        edgecolor="#7030A0", facecolor="#F3EEFF", zorder=2)
    ax.add_patch(motor_rect)
    ax.text((mx0 + mx1) / 2, (my0 + my1) / 2 + 0.4,
            "3-Phase\nBLDC Motor", ha="center", va="center",
            fontsize=10, fontname=FONT, fontweight="bold", color="#7030A0",
            zorder=10)
    ax.text((mx0 + mx1) / 2, (my0 + my1) / 2 - 0.55,
            "48V / 500W", ha="center", va="center",
            fontsize=8.5, fontname=FONT, color="#595959", zorder=10)

    # Motor terminals U, V, W
    term_ys = {"U": my0 + 2.5, "V": my0 + 2.0, "W": my0 + 1.5}
    for pname, px, _, _ in phases:
        sw_x, sw_y = sw_nodes[pname]
        ty = term_ys[pname]
        wire(ax, [(sw_x, sw_y), (15.5, sw_y), (15.5, ty), (mx0, ty)],
             color=PHASE_COLOR, lw=1.8)
        dot(ax, mx0, ty, color=PHASE_COLOR)
        label(ax, mx0 - 0.15, ty, pname, ha="right", fs=9, bold=True,
              color=PHASE_COLOR)

    # Gate driver blocks (simplified, one per phase)
    for pname, px, qhi, qlo in phases:
        gd_x = px - 1.8
        gd_y = 4.9
        rect2 = mpatches.FancyBboxPatch(
            (gd_x - 0.55, gd_y - 0.65), 1.1, 1.3,
            boxstyle="round,pad=0.08", linewidth=1.0,
            edgecolor=IC_EDGE, facecolor=IC_FILL, zorder=3)
        ax.add_patch(rect2)
        label(ax, gd_x, gd_y + 0.15, "IR2110/\nEG3013", fs=7, bold=True,
              color=IC_EDGE)
        label(ax, gd_x, gd_y - 0.4, "Gate Driver", fs=6.5, color="#595959")

        # Lines from driver to gates
        g_hi_pos = (px - 0.38 * h_mos - 0.38 * h_mos, hi_cy)
        g_lo_pos = (px - 0.38 * h_mos - 0.38 * h_mos, lo_cy)
        wire(ax, [(gd_x + 0.55, gd_y + 0.1),
                  (g_hi_pos[0] + 0.42, g_hi_pos[1])],
             color="#7030A0", lw=1.0)
        wire(ax, [(gd_x + 0.55, gd_y - 0.25),
                  (g_lo_pos[0] + 0.42, g_lo_pos[1])],
             color="#7030A0", lw=1.0)

    # Legend
    lx, ly = 17.0, 1.8
    ax.text(lx, ly + 0.6, "Legend", fontsize=8.5, fontname=FONT,
            fontweight="bold", color="#1F4E79")
    ax.plot([lx, lx + 0.5], [ly + 0.1, ly + 0.1], color=BUS_COLOR, lw=2)
    ax.text(lx + 0.6, ly + 0.1, "+48V Bus", fontsize=7.5, fontname=FONT,
            va="center")
    ax.plot([lx, lx + 0.5], [ly - 0.25, ly - 0.25], color=GND_COLOR, lw=2)
    ax.text(lx + 0.6, ly - 0.25, "GND Bus", fontsize=7.5, fontname=FONT,
            va="center")
    ax.plot([lx, lx + 0.5], [ly - 0.6, ly - 0.6], color=PHASE_COLOR, lw=2)
    ax.text(lx + 0.6, ly - 0.6, "Phase Wire", fontsize=7.5, fontname=FONT,
            va="center")

    fig.tight_layout(pad=0.3)
    for ext in ("svg", "png"):
        path = os.path.join(OUT, f"3phase_power_stage.{ext}")
        fig.savefig(path, dpi=150, bbox_inches="tight",
                    facecolor="white")
        print(f"  Saved: {path}")
    plt.close(fig)


# ══════════════════════════════════════════════════════════════════════════════
# SCHEMATIC 2 – Gate Driver Detail (one half-bridge, IR2110/EG3013)
# ══════════════════════════════════════════════════════════════════════════════
def draw_gate_driver():
    fig, ax = plt.subplots(figsize=(18, 13))
    ax.set_xlim(0, 18)
    ax.set_ylim(0, 13)
    ax.set_aspect("equal")
    ax.axis("off")
    fig.patch.set_facecolor("white")

    ax.text(9, 12.6, "Gate Driver Circuit – One Half-Bridge  |  IR2110 / EG3013",
            ha="center", va="center", fontsize=13, fontname=FONT,
            fontweight="bold", color="#1F4E79")
    ax.text(9, 12.2,
            "MOSFET: IRFB4110  |  R_gate = 10Ω  |  C_boot = 100nF/100V  "
            "|  D_boot = UF4007  |  R_gs = 10kΩ",
            ha="center", va="center", fontsize=9, fontname=FONT, color="#595959")

    bus_y, gnd_y = 11.2, 0.8
    px = 11.5    # MOSFET column x
    h_mos = 1.2

    # DC bus rails (right half)
    wire(ax, [(8.5, bus_y), (17.5, bus_y)], color=BUS_COLOR, lw=LW_BUS)
    wire(ax, [(8.5, gnd_y), (17.5, gnd_y)], color=GND_COLOR, lw=LW_BUS)
    label(ax, 17.6, bus_y, "+48V", ha="left", fs=10, bold=True, color=BUS_COLOR)
    label(ax, 17.6, gnd_y, "GND",  ha="left", fs=10, bold=True, color=GND_COLOR)

    # ── VCC rail (15V, for gate driver) ──
    vcc_y = 10.0
    wire(ax, [(1.0, vcc_y), (8.5, vcc_y)], color="#C55A11", lw=2.0)
    label(ax, 0.8, vcc_y, "VCC\n+15V", ha="right", fs=9, bold=True,
          color="#C55A11")

    # VCC decoupling cap
    wire(ax, [(3.0, vcc_y), (3.0, vcc_y - 0.08)])
    draw_capacitor(ax, 3.0, vcc_y - 0.35, lbl="100nF")
    wire(ax, [(3.0, vcc_y - 0.62), (3.0, gnd_y)], color=GND_COLOR, lw=1.2)
    label(ax, 3.0, vcc_y - 1.1, "C_vcc", fs=7.5, color="#595959")

    # ── IR2110 IC block ──
    ic_x, ic_y = 4.0, 5.8
    ic_w, ic_h = 2.8, 4.8
    ic_rect = mpatches.FancyBboxPatch(
        (ic_x, ic_y), ic_w, ic_h,
        boxstyle="round,pad=0.15", linewidth=1.8,
        edgecolor=IC_EDGE, facecolor=IC_FILL, zorder=3)
    ax.add_patch(ic_rect)
    label(ax, ic_x + ic_w / 2, ic_y + ic_h - 0.55,
          "IR2110 / EG3013", fs=9.5, bold=True, color=IC_EDGE)
    label(ax, ic_x + ic_w / 2, ic_y + ic_h - 1.0,
          "Half-Bridge Gate Driver", fs=7.5, color="#595959")

    # IC pins (label inside box)
    pins = [
        # (label_inside, side, y_offset_from_bottom, pin_direction)
        ("VCC",  "right", 4.0, "right"),
        ("HO",   "right", 3.0, "right"),
        ("VS",   "right", 2.2, "right"),
        ("LO",   "right", 1.2, "right"),
        ("COM",  "right", 0.4, "right"),
        ("HIN",  "left",  3.5, "left"),
        ("LIN",  "left",  2.5, "left"),
        ("SD",   "left",  1.5, "left"),
        ("VDD",  "left",  0.6, "left"),
    ]
    pin_coords = {}
    for pname, side, yoff, _ in pins:
        py = ic_y + yoff
        if side == "right":
            ax.text(ic_x + ic_w - 0.12, py, pname, ha="right", va="center",
                    fontsize=7.5, fontname=FONT, fontweight="bold",
                    color="#1F4E79", zorder=10)
            pin_coords[pname] = (ic_x + ic_w, py)
        else:
            ax.text(ic_x + 0.12, py, pname, ha="left", va="center",
                    fontsize=7.5, fontname=FONT, fontweight="bold",
                    color="#1F4E79", zorder=10)
            pin_coords[pname] = (ic_x, py)

    # VCC pin connection
    wire(ax, [(pin_coords["VCC"][0], pin_coords["VCC"][1]),
              (8.0, pin_coords["VCC"][1]),
              (8.0, vcc_y)], color="#C55A11", lw=1.6)
    dot(ax, 8.0, vcc_y, color="#C55A11")

    # COM pin → GND
    wire(ax, [(pin_coords["COM"][0], pin_coords["COM"][1]),
              (8.2, pin_coords["COM"][1]),
              (8.2, gnd_y)], color=GND_COLOR, lw=1.4)
    dot(ax, 8.2, gnd_y, color=GND_COLOR)

    # VDD pin → 5V (MCU logic)
    wire(ax, [(pin_coords["VDD"][0], pin_coords["VDD"][1]),
              (0.8, pin_coords["VDD"][1])])
    label(ax, 0.6, pin_coords["VDD"][1], "+5V\n(VDD)", ha="right", fs=8,
          color="#2E75B6")

    # MCU inputs (HIN, LIN, SD)
    for pname, xend, ylab in [("HIN", 0.6, "MCU\nHIN"),
                                ("LIN", 0.6, "MCU\nLIN"),
                                ("SD",  0.6, "MCU\nSD")]:
        py = pin_coords[pname][1]
        wire(ax, [(pin_coords[pname][0], py), (1.0, py)])
        # Arrow from MCU side
        ax.annotate("", xy=(pin_coords[pname][0], py),
                    xytext=(1.0, py),
                    arrowprops=dict(arrowstyle="-|>", color="#595959",
                                   lw=1.2, mutation_scale=8), zorder=4)
        label(ax, 0.5, py, ylab, ha="right", fs=8, color="#595959")

    # ── Bootstrap circuit ──
    # D_boot: VCC → VB through diode (anode at vcc rail side)
    boot_x = 9.5
    boot_vb_y = bus_y - 0.5  # VB node y
    # D_boot diode (anode=left/vcc, cathode=right/VB)
    wire(ax, [(8.0, vcc_y), (boot_x - 0.25, vcc_y)], color="#C55A11")
    # draw diode horizontally: anode at left, cathode at right
    dax_, day = boot_x - 0.25, vcc_y
    dbx, dby = boot_x + 0.25, vcc_y
    # horizontal diode
    tri = mpatches.Polygon([[dax_, day - 0.18], [dax_, day + 0.18],
                             [dbx, day]], closed=True,
                            facecolor="#FFF2CC", edgecolor=WIRE_COLOR, lw=1.2,
                            zorder=4)
    ax.add_patch(tri)
    ax.plot([dbx, dbx], [day - 0.18, day + 0.18],
            color=WIRE_COLOR, lw=1.5, zorder=4)
    label(ax, boot_x, day + 0.3, "D_boot\nUF4007", ha="center", fs=7.5)
    wire(ax, [(dbx, vcc_y), (px - 0.2, vcc_y),
              (px - 0.2, boot_vb_y)])   # VB node

    # C_boot: VB → VS
    vs_y = 7.2   # VS node (switching node)
    wire(ax, [(px - 0.2, boot_vb_y), (px - 0.2, boot_vb_y - 0.08)])
    draw_capacitor(ax, px - 0.2, boot_vb_y - 0.35, lbl="C_boot\n100nF/100V")
    wire(ax, [(px - 0.2, boot_vb_y - 0.62), (px - 0.2, vs_y)])
    label(ax, px - 0.55, boot_vb_y, "VB", ha="right", fs=8, bold=True,
          color="#C55A11")
    label(ax, px - 0.55, vs_y + 0.05, "VS", ha="right", fs=8, bold=True,
          color=PHASE_COLOR)

    # VS pin from IC → switching node
    wire(ax, [(pin_coords["VS"][0], pin_coords["VS"][1]),
              (8.5, pin_coords["VS"][1]),
              (8.5, vs_y),
              (px - 0.2, vs_y)], color=PHASE_COLOR, lw=1.5)

    # ── Hi-side MOSFET ──
    hi_cy = 8.8
    g_hi, d_hi, s_hi = draw_nmos(ax, px, hi_cy, h=h_mos, lbl="Q_hi\nIRFB4110")
    # Drain → 48V
    wire(ax, [(d_hi[0], d_hi[1]), (px, bus_y)], color=BUS_COLOR, lw=1.6)
    # Source → switching node (VS)
    wire(ax, [(s_hi[0], s_hi[1]), (px, vs_y)], color=PHASE_COLOR, lw=1.6)
    dot(ax, px, vs_y, color=PHASE_COLOR)
    dot(ax, px - 0.2, vs_y, color=PHASE_COLOR)

    # Rg_hi: HO pin → gate of Q_hi
    ho_y = pin_coords["HO"][1]
    ho_exit_x = pin_coords["HO"][0]
    # HO wire route: right of IC, up to gate height, then right
    gate_hi_x = g_hi[0]
    gate_hi_y = g_hi[1]
    rg_hi_x = (ho_exit_x + 0.5 + gate_hi_x) / 2
    wire(ax, [(ho_exit_x, ho_y), (ho_exit_x + 0.5, ho_y),
              (ho_exit_x + 0.5, gate_hi_y)])
    draw_resistor(ax, ho_exit_x + 0.5, gate_hi_y, gate_hi_x, gate_hi_y,
                  lbl="R_g\n10Ω")

    # R_gs_hi: gate to source pull-down
    wire(ax, [(gate_hi_x, gate_hi_y), (gate_hi_x - 0.3, gate_hi_y),
              (gate_hi_x - 0.3, s_hi[1] + 0.3)])
    draw_resistor(ax, gate_hi_x - 0.3, s_hi[1] + 0.3,
                  gate_hi_x - 0.3, s_hi[1] - 0.05, lbl="R_gs\n10kΩ")
    wire(ax, [(gate_hi_x - 0.3, s_hi[1] - 0.05),
              (gate_hi_x - 0.3, vs_y + 0.01)])

    # ── Lo-side MOSFET ──
    lo_cy = 3.5
    g_lo, d_lo, s_lo = draw_nmos(ax, px, lo_cy, h=h_mos, lbl="Q_lo\nIRFB4110")
    # Drain → switching node
    wire(ax, [(d_lo[0], d_lo[1]), (px, vs_y)], color=PHASE_COLOR, lw=1.6)
    dot(ax, px, vs_y)
    # Source → GND
    wire(ax, [(s_lo[0], s_lo[1]), (px, gnd_y)], color=GND_COLOR, lw=1.6)

    # Rg_lo
    lo_y = pin_coords["LO"][1]
    lo_exit_x = pin_coords["LO"][0]
    gate_lo_x = g_lo[0]
    gate_lo_y = g_lo[1]
    wire(ax, [(lo_exit_x, lo_y), (lo_exit_x + 0.4, lo_y),
              (lo_exit_x + 0.4, gate_lo_y)])
    draw_resistor(ax, lo_exit_x + 0.4, gate_lo_y, gate_lo_x, gate_lo_y,
                  lbl="R_g\n10Ω")

    # R_gs_lo
    wire(ax, [(gate_lo_x, gate_lo_y), (gate_lo_x - 0.3, gate_lo_y),
              (gate_lo_x - 0.3, s_lo[1] + 0.3)])
    draw_resistor(ax, gate_lo_x - 0.3, s_lo[1] + 0.3,
                  gate_lo_x - 0.3, s_lo[1] - 0.05, lbl="R_gs\n10kΩ")
    wire(ax, [(gate_lo_x - 0.3, s_lo[1] - 0.05),
              (gate_lo_x - 0.3, gnd_y)])
    dot(ax, gate_lo_x - 0.3, gnd_y, color=GND_COLOR)

    # Phase output
    out_x = 16.5
    wire(ax, [(px, vs_y), (out_x, vs_y)], color=PHASE_COLOR, lw=2.0)
    ax.annotate("", xy=(out_x + 0.4, vs_y), xytext=(out_x, vs_y),
                arrowprops=dict(arrowstyle="-|>", color=PHASE_COLOR,
                                lw=2, mutation_scale=12), zorder=5)
    label(ax, out_x + 0.6, vs_y + 0.25, "Phase Out\n(to Motor)", ha="left",
          fs=9, bold=True, color=PHASE_COLOR)

    # Heatsink note
    for cy_, qn in [(hi_cy, "Q_hi"), (lo_cy, "Q_lo")]:
        ax.text(px + 1.5, cy_, f"← Heatsink\n   (TO-220)\n   ≤5°C/W",
                ha="left", va="center", fontsize=7, fontname=FONT,
                color="#595959", style="italic")

    # Dead time note box
    note_rect = mpatches.FancyBboxPatch(
        (0.4, 10.8), 3.8, 1.5,
        boxstyle="round,pad=0.12", linewidth=1.0,
        edgecolor="#C00000", facecolor="#FCE4D6", zorder=3)
    ax.add_patch(note_rect)
    ax.text(2.3, 11.55,
            "!! Dead Time = 500 ns min\n"
            "HIN & LIN must NEVER be HIGH simultaneously\n"
            "Set in MCU PWM timer register",
            ha="center", va="center", fontsize=7.5, fontname=FONT,
            color="#C00000", zorder=10)

    fig.tight_layout(pad=0.3)
    for ext in ("svg", "png"):
        path = os.path.join(OUT, f"gate_driver_1phase.{ext}")
        fig.savefig(path, dpi=150, bbox_inches="tight", facecolor="white")
        print(f"  Saved: {path}")
    plt.close(fig)


if __name__ == "__main__":
    print("Drawing schematics...")
    draw_power_stage()
    draw_gate_driver()
    print("Done.")
