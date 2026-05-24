# -*- coding: utf-8 -*-
import sys, openpyxl
sys.stdout.reconfigure(encoding='utf-8')

wb = openpyxl.load_workbook(
    r'D:\work\202605_Trial_Claude_Code\01_Inverter_500W\claude_MOSFET_GateDriver_Selection_48V_500W.xlsx',
    data_only=True)

for sh in wb.sheetnames:
    ws = wb[sh]
    print(f'=== {sh} ({ws.max_row}r x {ws.max_column}c) ===')
    for row in ws.iter_rows(min_row=1, max_row=ws.max_row, values_only=True):
        if any(c is not None for c in row):
            print(list(row))
    print()
