#!/usr/bin/python

import csv

data = []
isProbablyHex = True

expected_header = ("Time [s]", "Type", "Src", "Dst", "Cmd", "Dat")
with open('einmal alles.csv') as csvfile:
    reader = csv.reader(csvfile)
    first_row = next(reader)
    if first_row is not expected_header:
        print ("Not expected header")
        print ("expected: " + str(expected_header))
        print ("actual  : " + str(first_row))

    for row in reader:
        # print(', '.join(row))
        time = float(row[0])
        type = row[1]
        src = int(row[2], 0)
        dst = int(row[3], 0)
        cmd = int(row[4], 0)
        dat = None
        if len(row) > 5 and "with data" in type:
            dat = int(row[5], 0)
        parsed_row = (time, type, src, dst, cmd, dat)
        data.append(parsed_row)


unique_src = set(row[2] for row in data)
unique_dst = set(row[3] for row in data)
unique_cmd = set(row[4] for row in data)

print ("Unique sources:")
for src in unique_src:
    print ("\t" + hex(src))
print ("Unique destinations:")
for dst in unique_dst:
    print ("\t" + hex(dst))
print ("Unique commands:")
for cmd in unique_cmd:
    print ("\t" + hex(cmd))
