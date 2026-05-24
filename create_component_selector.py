from openpyxl import Workbook
from openpyxl.styles import (Font, PatternFill, Alignment, Border, Side,
                              GradientFill)
from openpyxl.utils import get_column_letter
from openpyxl.styles.numbers import FORMAT_PERCENTAGE_00

wb = Workbook()

# ── Color palette ──────────────────────────────────────────────────────────────
HDR_FILL   = PatternFill("solid", start_color="1F4E79")   # dark blue
SUB_FILL   = PatternFill("solid", start_color="2E75B6")   # medium blue
ALT_FILL   = PatternFill("solid", start_color="D6E4F0")   # light blue row
WHITE_FILL = PatternFill("solid", start_color="FFFFFF")
GREEN_FILL = PatternFill("solid", start_color="E2EFDA")   # recommended
YELLOW_FILL= PatternFill("solid", start_color="FFF2CC")   # caution
RED_FILL   = PatternFill("solid", start_color="FCE4D6")   # not recommended
CALC_FILL  = PatternFill("solid", start_color="F2F2F2")   # calc background

HDR_FONT   = Font(name="Arial", bold=True, color="FFFFFF", size=10)
TITLE_FONT = Font(name="Arial", bold=True, color="FFFFFF", size=13)
BODY_FONT  = Font(name="Arial", size=9)
BOLD_FONT  = Font(name="Arial", bold=True, size=9)
BOLD_DARK  = Font(name="Arial", bold=True, size=9, color="1F4E79")

thin = Side(style="thin", color="BFBFBF")
med  = Side(style="medium", color="2E75B6")
THIN_BORDER = Border(left=thin, right=thin, top=thin, bottom=thin)
MED_BORDER  = Border(left=med,  right=med,  top=med,  bottom=med)

CENTER = Alignment(horizontal="center", vertical="center", wrap_text=True)
LEFT   = Alignment(horizontal="left",   vertical="center", wrap_text=True)
RIGHT  = Alignment(horizontal="right",  vertical="center")


def style_cell(cell, font=None, fill=None, align=None, border=None,
               num_fmt=None):
    if font:   cell.font      = font
    if fill:   cell.fill      = fill
    if align:  cell.alignment = align
    if border: cell.border    = border
    if num_fmt: cell.number_format = num_fmt


def set_col_width(ws, col_letter, width):
    ws.column_dimensions[col_letter].width = width


def merge_title(ws, cell_range, text, fill=None, font=None):
    ws.merge_cells(cell_range)
    c = ws[cell_range.split(":")[0]]
    c.value = text
    style_cell(c, font=font or TITLE_FONT, fill=fill or HDR_FILL,
               align=CENTER, border=MED_BORDER)


# ══════════════════════════════════════════════════════════════════════════════
# SHEET 1 – Design Parameters
# ══════════════════════════════════════════════════════════════════════════════
ws_param = wb.active
ws_param.title = "Design Parameters"
ws_param.sheet_view.showGridLines = False
ws_param.row_dimensions[1].height = 30
ws_param.row_dimensions[2].height = 6

merge_title(ws_param, "A1:F1",
            "🔧  Inverter Design Parameters  |  48 V / 500 W Motor Drive")

# Section header
ws_param["A3"] = "DESIGN INPUTS"
ws_param["A3"].font = Font(name="Arial", bold=True, size=10, color="FFFFFF")
ws_param["A3"].fill = SUB_FILL
ws_param["A3"].alignment = CENTER
ws_param.merge_cells("A3:F3")

params_hdr = ["Parameter", "Symbol", "Value", "Unit", "Notes", "Formula / Source"]
for ci, h in enumerate(params_hdr, 1):
    c = ws_param.cell(row=4, column=ci, value=h)
    style_cell(c, font=HDR_FONT, fill=HDR_FILL, align=CENTER, border=THIN_BORDER)

params = [
    ("Motor Voltage (Bus)",          "V_bus",   48,    "V",    "Nominal 48 V LiFePO4 / Lead-acid", "—"),
    ("Output Power",                 "P_out",   500,   "W",    "Continuous rated power",            "—"),
    ("Motor Efficiency (est.)",      "η_motor",  0.85, "—",    "Typical BLDC efficiency",           "—"),
    ("Inverter Efficiency (est.)",   "η_inv",    0.95, "—",    "PWM inverter typical",              "—"),
    ("Continuous DC Current",        "I_dc",    None,  "A",    "=P_out/(V_bus×η_motor×η_inv)",      "=C5/(C4*C6*C7)"),
    ("Phase Peak Current (3-phase)", "I_peak",  None,  "A",    "I_dc × √2 × 1/√3 × 2 (margin)",    "=C8*1.5"),
    ("MOSFET Voltage Rating",        "V_ds_min",None,  "V",    "V_bus × 2.0 safety margin",         "=C4*2"),
    ("MOSFET Current Rating",        "I_d_min", None,  "A",    "I_peak × 2.0 safety margin",        "=C9*2"),
    ("Switching Frequency",          "f_sw",    20,    "kHz",  "Typical BLDC inverter",             "—"),
    ("Dead Time",                    "t_dead",  500,   "ns",   "Prevent shoot-through",             "—"),
]

fills_p = [WHITE_FILL, WHITE_FILL, WHITE_FILL, WHITE_FILL,
           CALC_FILL, CALC_FILL, CALC_FILL, CALC_FILL,
           WHITE_FILL, WHITE_FILL]

for ri, (row, fill) in enumerate(zip(params, fills_p), 5):
    name, sym, val, unit, note, formula = row
    data = [name, sym, formula if formula.startswith("=") else val, unit, note,
            formula if not formula.startswith("=") else ""]
    for ci, v in enumerate(data, 1):
        c = ws_param.cell(row=ri, column=ci, value=v)
        style_cell(c, font=BODY_FONT, fill=fill, align=LEFT if ci in (1,5,6) else CENTER,
                   border=THIN_BORDER)

# Format percentage rows
for row in [7, 8]:
    ws_param.cell(row=row, column=3).number_format = "0%"
for row in [8, 9, 10, 11]:
    ws_param.cell(row=row, column=3).number_format = "0.0"

# Column widths
for col, w in zip("ABCDEF", [32, 16, 12, 8, 42, 28]):
    set_col_width(ws_param, col, w)


# ══════════════════════════════════════════════════════════════════════════════
# SHEET 2 – MOSFET Selection
# ══════════════════════════════════════════════════════════════════════════════
ws_mos = wb.create_sheet("MOSFET Selection")
ws_mos.sheet_view.showGridLines = False
ws_mos.row_dimensions[1].height = 30
ws_mos.row_dimensions[2].height = 6

merge_title(ws_mos, "A1:M1",
            "⚡  MOSFET Selection  |  48 V / 500 W Motor Inverter  |  หาซื้อได้ใน Shopee ไทย")

# Sub-header note
ws_mos.merge_cells("A2:M2")
c2 = ws_mos["A2"]
c2.value = ("★ = แนะนำ  |  ✔ = ใช้ได้  |  ✖ = ไม่แนะนำ  |  "
            "สีเขียว = แนะนำ, สีเหลือง = ใช้ได้แต่มีข้อจำกัด, สีแดง = ไม่เหมาะสม")
c2.font  = Font(name="Arial", bold=True, size=9, color="1F4E79")
c2.fill  = PatternFill("solid", start_color="D6E4F0")
c2.alignment = CENTER

# Column headers
mos_hdrs = [
    "Part Number", "Manufacturer", "V_ds (V)", "I_d cont. (A)",
    "I_d peak (A)", "R_ds(on) (mΩ)", "V_gs(th) (V)", "Package",
    "Gate Charge Qg (nC)", "ราคา/ตัว (฿ est.)",
    "เลือกใช้", "หมายเหตุ / ข้อดีข้อเสีย", "ค้นหาใน Shopee"
]
for ci, h in enumerate(mos_hdrs, 1):
    c = ws_mos.cell(row=3, column=ci, value=h)
    style_cell(c, font=HDR_FONT, fill=HDR_FILL, align=CENTER, border=THIN_BORDER)
ws_mos.row_dimensions[3].height = 36

# MOSFET data
# rec = recommended(green), ok = yellow, no = red
mosdata = [
    # Part, Maker, Vds, Id, Idpk, Rds(mΩ), Vgsth, Pkg, Qg, Price, rating, note, shopee
    ("IRFB4110",   "Infineon/IR",  100, 120, 400,  3.7,  "2.0-4.0", "TO-220AB", 150,  "40-80",  "★ แนะนำ",
     "Rds ต่ำมาก ทนกระแสสูง เหมาะที่สุดสำหรับงานนี้ ราคาปานกลาง",
     'IRFB4110 MOSFET'),

    ("IRF3710",    "Infineon/IR",  100,  57, 200, 23.0,  "2.0-4.0", "TO-220AB",  71,  "25-50",  "★ แนะนำ",
     "ราคาถูก หาง่าย Rds พอใช้ เหมาะสำหรับงบจำกัด",
     'IRF3710 MOSFET'),

    ("IRFP260N",   "Infineon/IR",  200,  50, 120, 40.0,  "2.0-4.0", "TO-247AC",  97,  "60-120", "✔ ใช้ได้",
     "Vds สูง margin เยอะ แต่ Rds สูงกว่า ความร้อนมากกว่า",
     'IRFP260N MOSFET'),

    ("NCE3080K",   "NCE (TH)",      80,  30,  80, 28.0,  "1.5-3.5", "TO-220",    58,  "15-35",  "✔ ใช้ได้",
     "ราคาถูกมาก หาง่ายใน Shopee แต่ Vds=80V margin น้อย และ Id ต้องขนาน",
     'NCE3080K MOSFET'),

    ("IRF540N",    "Infineon/IR",  100,  33,  94, 77.0,  "2.0-4.0", "TO-220AB",  72,  "20-45",  "✔ ใช้ได้",
     "ราคาถูก หาง่ายมาก แต่ Rds สูง ต้องขนาน 2 ตัว/ขา ความร้อนสูง",
     'IRF540N MOSFET'),

    ("STP75NF75",  "STMicro",       75,  75, 300,  8.0,  "2.0-4.0", "TO-220",   100,  "35-70",  "✔ ใช้ได้",
     "Vds=75V ค่อนข้างน้อยสำหรับ 48V (margin 56%) ควรระวัง spike",
     'STP75NF75 MOSFET'),

    ("IRFZ44N",    "Infineon/IR",   55,  49, 160, 17.5,  "2.0-4.0", "TO-220AB",  88,  "15-30",  "✖ ไม่แนะนำ",
     "Vds=55V ต่ำเกินไป! สำหรับ 48V bus มี margin เพียง 14% เสียง breakdown",
     'IRFZ44N MOSFET'),

    ("IRLZ44N",    "Infineon/IR",   55,  47, 160, 22.0,  "1.0-2.0", "TO-220AB",  90,  "20-40",  "✖ ไม่แนะนำ",
     "Vds=55V ต่ำเกินไปสำหรับ 48V Logic-level gate แต่ใช้ Vds ไม่ได้",
     'IRLZ44N MOSFET'),

    ("FDP2532",    "ON Semi",      150,  90, 300, 12.0,  "2.0-4.0", "TO-220",   130,  "50-90",  "✔ ใช้ได้",
     "Vds สูง margin เยอะ Rds ดี แต่หายากกว่าในไทย ราคาสูงขึ้น",
     'FDP2532 MOSFET'),
]

row_fills = {
    "★ แนะนำ":      GREEN_FILL,
    "✔ ใช้ได้":     YELLOW_FILL,
    "✖ ไม่แนะนำ":  RED_FILL,
}

for ri, row in enumerate(mosdata, 4):
    fill = row_fills.get(row[10], WHITE_FILL)
    for ci, val in enumerate(row, 1):
        c = ws_mos.cell(row=ri, column=ci, value=val)
        align = LEFT if ci in (12,) else CENTER
        style_cell(c, font=BODY_FONT, fill=fill, align=align, border=THIN_BORDER)
    ws_mos.row_dimensions[ri].height = 42

# Rating column bold
for ri in range(4, 4 + len(mosdata)):
    ws_mos.cell(row=ri, column=11).font = BOLD_FONT

# Column widths
mos_widths = [14, 14, 10, 14, 14, 14, 12, 10, 16, 14, 12, 48, 28]
for ci, w in enumerate(mos_widths, 1):
    set_col_width(ws_mos, get_column_letter(ci), w)

# Design threshold note
note_row = 4 + len(mosdata) + 1
ws_mos.merge_cells(f"A{note_row}:M{note_row}")
nc = ws_mos[f"A{note_row}"]
nc.value = ("📌  เกณฑ์การเลือก: V_ds ≥ 96V (48V×2.0), I_d ≥ 20A (continuous DC ~11A × 2.0 margin)  "
            "|  ขนาดหม้อน้ำ (heatsink) ขึ้นกับ R_ds(on) × I²  "
            "|  ควรขนาน MOSFET 2 ตัว/ขาสำหรับ Rds สูง")
nc.font  = Font(name="Arial", italic=True, size=8, color="595959")
nc.fill  = PatternFill("solid", start_color="F2F2F2")
nc.alignment = LEFT


# ══════════════════════════════════════════════════════════════════════════════
# SHEET 3 – Gate Driver Selection
# ══════════════════════════════════════════════════════════════════════════════
ws_gd = wb.create_sheet("Gate Driver Selection")
ws_gd.sheet_view.showGridLines = False
ws_gd.row_dimensions[1].height = 30
ws_gd.row_dimensions[2].height = 6

merge_title(ws_gd, "A1:L1",
            "🔌  Gate Driver Selection  |  48 V Inverter  |  หาซื้อได้ใน Shopee ไทย")

ws_gd.merge_cells("A2:L2")
c2g = ws_gd["A2"]
c2g.value = ("★ = แนะนำ  |  ✔ = ใช้ได้  |  "
             "ใช้คู่กับ Bootstrap capacitor 100–470 nF สำหรับ High-side drive")
c2g.font  = Font(name="Arial", bold=True, size=9, color="1F4E79")
c2g.fill  = PatternFill("solid", start_color="D6E4F0")
c2g.alignment = CENTER

gd_hdrs = [
    "Part Number", "Manufacturer", "ประเภท", "V_offset max (V)",
    "Source (A)", "Sink (A)", "Propagation Delay (ns)",
    "ราคา/ตัว (฿ est.)", "Isolation", "เลือกใช้",
    "หมายเหตุ / วิธีใช้", "ค้นหาใน Shopee"
]
for ci, h in enumerate(gd_hdrs, 1):
    c = ws_gd.cell(row=3, column=ci, value=h)
    style_cell(c, font=HDR_FONT, fill=HDR_FILL, align=CENTER, border=THIN_BORDER)
ws_gd.row_dimensions[3].height = 36

gddata = [
    ("IR2110",  "Infineon/IR",  "Half-Bridge\n(Hi+Lo side)", 600, 2.0, 2.0,  120,
     "30–60",  "ไม่มี (Bootstrap)", "★ แนะนำ",
     "ยอดนิยมที่สุด ใช้ Bootstrap สำหรับ Hi-side ต้องการ cap 100nF ต่อ 1 ขา ใช้ 3 ชิป/3-phase",
     "IR2110 gate driver"),

    ("IR2104",  "Infineon/IR",  "Half-Bridge\n(Hi+Lo side)", 600, 0.6, 0.6,  680,
     "20–40",  "ไม่มี (Bootstrap)", "★ แนะนำ",
     "มี SD pin ในตัว เล็กกว่า IR2110 กระแส drive น้อยกว่า เหมาะงบจำกัด/MOSFET Qg ต่ำ",
     "IR2104 gate driver"),

    ("EG3013",  "EG Semi (CN)","Half-Bridge\n(Hi+Lo side)", 600, 1.5, 1.5,  150,
     "10–25",  "ไม่มี (Bootstrap)", "★ แนะนำ",
     "Clone IR2110 ราคาถูกมาก pinout เหมือนกัน หาง่ายมากใน Shopee คุณภาพดีสำหรับต้นแบบ",
     "EG3013 gate driver"),

    ("IR2101",  "Infineon/IR",  "Half-Bridge\n(Hi+Lo side)", 600, 0.2, 0.2,  680,
     "15–30",  "ไม่มี (Bootstrap)", "✔ ใช้ได้",
     "กระแสต่ำมาก (200mA) ต้องเพิ่ม totem-pole transistor คู่ถ้า MOSFET Qg > 50nC",
     "IR2101 gate driver"),

    ("TLP250",  "Toshiba",      "Single channel\n(Isolated)", 35,  1.5, 1.5,  500,
     "25–55",  "Opto (2500V)",    "✔ ใช้ได้",
     "Isolated ใช้ได้ทั้ง Hi/Lo side แต่ต้องใช้ไฟ isolated ต่อขา ราคาสูงกว่า ใช้ 6 ชิป",
     "TLP250 gate driver opto"),

    ("TC4420",  "Microchip",    "Single Lo-side\nBuffer",     18,  6.0, 6.0,   45,
     "40–80",  "ไม่มี",            "✔ ใช้ได้",
     "Low-side เท่านั้น กระแสสูง delay ต่ำ ใช้เป็น driver เสริมต่อจาก IR2110 เพื่อเพิ่ม กระแส",
     "TC4420 mosfet driver"),

    ("HCPL3120","Broadcom",     "Single channel\n(Isolated)", 30,  2.5, 2.5,  500,
     "40–80",  "Opto (1414V)",    "✔ ใช้ได้",
     "Isolated กระแสสูงกว่า TLP250 เหมาะงานที่ต้องการ isolation สูง ต้องการไฟแยก/ขา",
     "HCPL3120 HCPL-3120 gate driver"),

    ("DRV8302", "TI",           "3-Phase\nIntegrated",        60,  1.7, 2.3,  200,
     "150–300","ไม่มี",            "✔ ใช้ได้",
     "ไดร์ฟ 3 เฟสในชิปเดียว มี Current sensing OCP ในตัว แต่ราคาสูง หายากใน Shopee",
     "DRV8302 gate driver"),
]

for ri, row in enumerate(gddata, 4):
    rating = row[9]
    fill   = row_fills.get(rating, WHITE_FILL)
    for ci, val in enumerate(row, 1):
        c = ws_gd.cell(row=ri, column=ci, value=val)
        align = LEFT if ci in (11,) else CENTER
        style_cell(c, font=BODY_FONT, fill=fill, align=align, border=THIN_BORDER)
    ws_gd.row_dimensions[ri].height = 46
    ws_gd.cell(row=ri, column=10).font = BOLD_FONT

gd_widths = [12, 14, 16, 16, 10, 10, 18, 14, 16, 12, 52, 32]
for ci, w in enumerate(gd_widths, 1):
    set_col_width(ws_gd, get_column_letter(ci), w)

note_row2 = 4 + len(gddata) + 1
ws_gd.merge_cells(f"A{note_row2}:L{note_row2}")
nc2 = ws_gd[f"A{note_row2}"]
nc2.value = ("📌  สำหรับ 3-phase inverter: ใช้ IR2110/EG3013 จำนวน 3 ชิป (1 ชิป/เฟส)  "
             "|  Bootstrap cap แนะนำ 100–470 nF/ขา (ทนแรงดัน ≥100V)  "
             "|  Gate resistor 10–22 Ω ลด EMI และ ringing")
nc2.font  = Font(name="Arial", italic=True, size=8, color="595959")
nc2.fill  = PatternFill("solid", start_color="F2F2F2")
nc2.alignment = LEFT


# ══════════════════════════════════════════════════════════════════════════════
# SHEET 4 – BOM Summary (recommended set)
# ══════════════════════════════════════════════════════════════════════════════
ws_bom = wb.create_sheet("BOM (Recommended)")
ws_bom.sheet_view.showGridLines = False
ws_bom.row_dimensions[1].height = 30
ws_bom.row_dimensions[2].height = 6

merge_title(ws_bom, "A1:H1",
            "📋  BOM – ชุดแนะนำสำหรับ Inverter 3-Phase 48V / 500W")

bom_hdrs = ["#", "รายการ", "Part Number", "Qty", "ราคา/หน่วย (฿ est.)",
            "รวม (฿)", "หมายเหตุ", "ค้นหาใน Shopee"]
for ci, h in enumerate(bom_hdrs, 1):
    c = ws_bom.cell(row=3, column=ci, value=h)
    style_cell(c, font=HDR_FONT, fill=HDR_FILL, align=CENTER, border=THIN_BORDER)
ws_bom.row_dimensions[3].height = 30

bom_items = [
    (1, "MOSFET – Power Switch",       "IRFB4110",   6,  60, "=D4*E4",  "6 ตัว สำหรับ 3-phase H-bridge TO-220",   "IRFB4110"),
    (2, "Gate Driver IC",              "EG3013",      3,  20, "=D5*E5",  "1 ชิปต่อ 1 เฟส (Hi+Lo side)",           "EG3013 gate driver"),
    (3, "Bootstrap Capacitor",         "100nF 100V",  6,   2, "=D6*E6",  "1 ตัวต่อ Hi-side ทนแรงดัน ≥100V",       "capacitor 100nF 100V"),
    (4, "Bootstrap Diode",             "UF4007",      3,   5, "=D7*E7",  "Fast recovery 1A/1000V ต่อ Hi-side",    "UF4007 diode"),
    (5, "Gate Resistor (Hi-side)",     "10Ω 0.5W",   6,   1, "=D8*E8",  "จำกัดกระแส gate ลด ringing",            "resistor 10 ohm"),
    (6, "Gate Resistor (Lo-side)",     "10Ω 0.5W",   6,   1, "=D9*E9",  "จำกัดกระแส gate ลด ringing",            "resistor 10 ohm"),
    (7, "Gate Pull-down Resistor",     "10kΩ 0.25W", 6,   1, "=D10*E10","ดึง gate ลด float เมื่อ driver ไม่ active","resistor 10k ohm"),
    (8, "Decoupling Cap (driver VCC)", "100nF 25V",  3,   2, "=D11*E11","ต่อใกล้ขา VCC ของ IC",                  "capacitor 100nF"),
    (9, "Heatsink (MOSFET)",           "TO-220 fin",  6,  20, "=D12*E12","Rth ≤ 5 °C/W สำหรับ dissipation ~3W/ตัว","heatsink TO-220"),
]

for ri, row in enumerate(bom_items, 4):
    fill = ALT_FILL if ri % 2 == 0 else WHITE_FILL
    for ci, val in enumerate(row, 1):
        c = ws_bom.cell(row=ri, column=ci, value=val)
        align = LEFT if ci in (2, 7) else CENTER
        style_cell(c, font=BODY_FONT, fill=fill, align=align, border=THIN_BORDER)
    ws_bom.row_dimensions[ri].height = 28

# Total row
total_row = 4 + len(bom_items)
ws_bom.merge_cells(f"A{total_row}:E{total_row}")
tc = ws_bom[f"A{total_row}"]
tc.value = "ประมาณการรวม (฿)"
style_cell(tc, font=BOLD_FONT, fill=HDR_FILL, align=RIGHT, border=THIN_BORDER)
tc.font = Font(name="Arial", bold=True, size=10, color="FFFFFF")

sum_cell = ws_bom.cell(row=total_row, column=6,
                       value=f"=SUM(F4:F{total_row-1})")
style_cell(sum_cell, font=Font(name="Arial", bold=True, size=10),
           fill=GREEN_FILL, align=CENTER, border=THIN_BORDER)
sum_cell.number_format = '#,##0'

for ci in [7, 8]:
    c = ws_bom.cell(row=total_row, column=ci, value="")
    style_cell(c, fill=HDR_FILL, border=THIN_BORDER)

# Format cost columns
for ri in range(4, total_row):
    ws_bom.cell(row=ri, column=5).number_format = '#,##0'
    ws_bom.cell(row=ri, column=6).number_format = '#,##0'

bom_widths = [5, 28, 16, 8, 18, 12, 42, 30]
for ci, w in enumerate(bom_widths, 1):
    set_col_width(ws_bom, get_column_letter(ci), w)


# ══════════════════════════════════════════════════════════════════════════════
# SHEET 5 – Wiring Diagram Description
# ══════════════════════════════════════════════════════════════════════════════
ws_circ = wb.create_sheet("Gate Drive Circuit")
ws_circ.sheet_view.showGridLines = False
ws_circ.row_dimensions[1].height = 30

merge_title(ws_circ, "A1:D1",
            "📐  Gate Drive Circuit – คำอธิบายวงจรและการต่อ (IR2110 / EG3013)")

circuit_notes = [
    ("", ""),
    ("─── POWER SUPPLY ───", ""),
    ("VDD (IC logic)",    "5V หรือ 3.3V จาก MCU supply"),
    ("VCC (driver)",      "12–15 V สำหรับ drive MOSFET gate (แยกจาก 48V bus)"),
    ("COM",               "GND ร่วมของ Low-side และ MCU"),
    ("", ""),
    ("─── BOOTSTRAP CIRCUIT (Hi-side) ───", ""),
    ("VB",  "Bootstrap voltage = VCC + VS (floating, ตาม source Hi-side MOSFET)"),
    ("VS",  "ต่อกับ Source ของ Hi-side MOSFET (switching node)"),
    ("Bootstrap diode (D_boot)", "UF4007 ต่อจาก VCC → VB ผ่าน diode (cathode→VB, anode→VCC)"),
    ("Bootstrap cap (C_boot)",   "100–470 nF ต่อระหว่าง VB–VS (ทนแรงดัน ≥ V_bus + VCC = 60V → ใช้ 100V)"),
    ("", ""),
    ("─── GATE RESISTORS ───", ""),
    ("R_g_on  (10Ω)",  "ต่ออนุกรมระหว่าง HO/LO output กับ Gate MOSFET → ชะลอ turn-on ลด di/dt"),
    ("R_g_off (0Ω–10Ω)","บางวงจรแยก R สำหรับ turn-off เพื่อ trade-off ระหว่าง loss กับ EMI"),
    ("R_gs   (10kΩ)",  "ต่อระหว่าง Gate–Source ของ MOSFET → ป้องกัน gate float เมื่อ driver ไม่ active"),
    ("", ""),
    ("─── DEAD TIME ───", ""),
    ("ตั้ง Dead Time",  "500 ns จาก MCU firmware หรือ Timer register (ห้าม HI+LO=ON พร้อมกัน)"),
    ("", ""),
    ("─── DECOUPLING ───", ""),
    ("C_vcc (100nF + 10µF)", "ต่อใกล้ขา VCC-COM ของ IC ทุกตัว ป้องกัน voltage dip ตอน switching"),
    ("", ""),
    ("─── INPUT SIGNALS ───", ""),
    ("HIN",  "High-side input จาก MCU PWM (active HIGH)"),
    ("LIN",  "Low-side input จาก MCU PWM (active HIGH)"),
    ("SD",   "Shutdown (IR2104/IR2112) → active LOW หรือ HIGH ขึ้นกับรุ่น อ่าน datasheet"),
    ("", ""),
    ("─── THERMAL ───", ""),
    ("Heatsink",  "ต้องการ Rth ≤ (T_j_max – T_amb) / P_loss ต่อตัว"),
    ("P_loss/ตัว (แนะนำ)", "≈ I²×Rds×0.5 (conduction) + 0.5×Qg×V_gs×f_sw (switching) ≈ 2–5 W/ตัว"),
    ("Thermal paste", "ทาระหว่าง MOSFET กับ heatsink ลด Rth_contact"),
]

for ri, (item, desc) in enumerate(circuit_notes, 3):
    is_hdr = item.startswith("───")
    c1 = ws_circ.cell(row=ri, column=1, value=item)
    c2 = ws_circ.cell(row=ri, column=2, value=desc)
    if is_hdr:
        style_cell(c1, font=Font(name="Arial", bold=True, size=9, color="1F4E79"),
                   fill=PatternFill("solid", start_color="D6E4F0"), border=THIN_BORDER)
        ws_circ.merge_cells(f"A{ri}:D{ri}")
        c1.alignment = LEFT
    else:
        style_cell(c1, font=BOLD_DARK, fill=CALC_FILL if ri%2==0 else WHITE_FILL,
                   align=LEFT, border=THIN_BORDER)
        style_cell(c2, font=BODY_FONT, fill=CALC_FILL if ri%2==0 else WHITE_FILL,
                   align=LEFT, border=THIN_BORDER)
        ws_circ.merge_cells(f"B{ri}:D{ri}")
    ws_circ.row_dimensions[ri].height = 20

set_col_width(ws_circ, "A", 32)
set_col_width(ws_circ, "B", 60)
set_col_width(ws_circ, "C", 20)
set_col_width(ws_circ, "D", 20)


# ══════════════════════════════════════════════════════════════════════════════
# Save
# ══════════════════════════════════════════════════════════════════════════════
out_path = r"D:\work\202605_Trial_Claude_Code\01_Inverter_500W\MOSFET_GateDriver_Selection_48V_500W.xlsx"
wb.save(out_path)
print(f"Saved: {out_path}")
