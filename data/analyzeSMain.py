#!/usr/bin/env python
import csv
import sys
import os
from datetime import date

directoryList = os.listdir(sys.argv[1])
files = []
for entry in directoryList:
	if entry[-4: ] == '.txt' and entry[: 2] == 's_':
		files.append(entry)
data = []

for file in files:
	f = open(file, 'rt')
	try:
		starts = {}
		ends = {}
		happens = {}
		reader = csv.reader(f, delimiter = " ", dialect = csv.excel_tab)
		for row in reader:
			if len(row) == 3:
				if row[0] == 'bsocket':
					if row[1] not in starts:
						starts[row[1]] = int(row[2])
						happens[row[1]] = 1
					else:
						starts[row[1]] = starts[row[1]] + int(row[2])
						happens[row[1]] = happens[row[1]] + 1
				if row[0] == 'esocket':
					if row[1] not in ends:
						ends[row[1]] = int(row[2])
					else:
						ends[row[1]] = ends[row[1]] + int(row[2])
			else:
				pass
	finally:
		pass
	results = {key: ends[key] - starts.get(key, 0) for key in starts.keys()}
	avg = {key: results[key]/float(happens.get(key,0)) for key in results.keys()}
	data.append(avg.values())

with open('outputSMain.csv', 'w+') as fw:
	writer = csv.writer(fw)
	writer.writerows(data)