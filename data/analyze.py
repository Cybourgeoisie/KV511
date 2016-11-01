#!/usr/bin/env python
#consolidate.py
import csv
import sys
import os
from datetime import date

def rotate(l, n):
    return l[n:] + l[:n]

directoryList = os.listdir(sys.argv[1])
files = []
for entry in directoryList:
	if entry[-4:] == '.txt':
		files.append(entry)
data = []

for file in files:
	f = open(file, 'rt')
	try:
		starts = []
		ends = []
		results = []
		reader = csv.reader(f, delimiter=" ",dialect=csv.excel_tab)
		for row in reader:
			if len(row)==2:
				if row[0][0] == 'b':
					starts.append(row[1])
				if row[0][0] == 'e':
					ends.append(row[1])
		ends = rotate(ends, -1)
		results = [int(a_i) - int(b_i) for a_i, b_i in zip(ends, starts)]
		results.insert(0, file)
		results = map(str, results)
		data.append(results)
	finally:
		f.close()

with open('output.csv', 'w+') as fw:
	writer = csv.writer(fw)
	writer.writerows(data)
