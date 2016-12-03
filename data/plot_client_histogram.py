import re
import os
import sys
import csv
import numpy as np
import matplotlib.pyplot as plt

# Determine which data we want to look at
results_dir = sys.argv[1]
if not os.path.isdir('./' + results_dir):
	raise Exception("Results folder not found")

# Collect all file data
client_all_mt     = {'1':[],'5':[],'10':[],'15':[],'20':[]}
client_session_mt = {'1':[],'5':[],'10':[],'15':[],'20':[]}
client_all_as     = {'1':[],'5':[],'10':[],'15':[],'20':[]}
client_session_as = {'1':[],'5':[],'10':[],'15':[],'20':[]}

# For all possible folders within the results folder
for folder in os.listdir('./' + results_dir):
	
	# Gather all of the output
	client_file = './' + results_dir + '/' + folder + '/outputCT.csv'

	# Get the current number from the folder name
	folder_items = folder.split('_')
	num = str(folder_items[-1])

	if os.path.exists(client_file):
		with open(client_file) as csvfile:
	
			# Read the data as a CSV
			client_data = csv.reader(csvfile, delimiter=',')
			for row in client_data:
				for key, el in enumerate(row):
					# analyzeSMain
						# All elements are averages for socket times
					# analyzeCT and analyzeST
						# 0th element is file. Ignore for now.
						# 1st element is total time elapsed.
						# 2+K elements are session times (client) or message times (server)
					if key == 1:
						if "_mt_t1" in folder:
							client_all_mt['1'].append(int(el))
						elif "_as_t1" in folder:
							client_all_as['1'].append(int(el))
						elif "_mt_" in folder:
							client_all_mt[num].append(int(el))
						else:
							client_all_as[num].append(int(el))
					elif key > 1:
						if "_mt_t1" in folder:
							client_session_mt['1'].append(int(el))
						elif "_as_t1" in folder:
							client_session_as['1'].append(int(el))
						elif "_mt_" in folder:
							client_session_mt[num].append(int(el))
						else:
							client_session_as[num].append(int(el))

client_session_mt_means = [
	np.mean(client_session_mt['1']),
	np.mean(client_session_mt['5']),
	np.mean(client_session_mt['10']),
	np.mean(client_session_mt['15']),
	np.mean(client_session_mt['20'])
]

client_session_as_means = [
	np.mean(client_session_as['1']),
	np.mean(client_session_as['5']),
	np.mean(client_session_as['10']),
	np.mean(client_session_as['15']),
	np.mean(client_session_as['20'])
]

# Plot the results
plt.figure(1)

#data = np.vstack([client_session_mt['1'], client_session_as['1']]).T
x = [1,5,10,15,20]
width = 1/1.5

data = client_session_as_means
plt.bar(x, data, width, color="red", alpha=0.5, label="Asynchronous")

x = [1+width,5+width,10+width,15+width,20+width]
data = client_session_mt_means
plt.bar(x, data, width, color="blue", alpha=0.5, label="Multithreaded")
plt.legend(loc='best')
plt.title("Time to Serve Session on Client-Side")
plt.ylabel("Time (ns)")
plt.xlabel("# of Simultaneous Client Threads")

plt.show()
plt.clf()


client_all_mt_means = [
	np.mean(client_all_mt['1']),
	np.mean(client_all_mt['5']),
	np.mean(client_all_mt['10']),
	np.mean(client_all_mt['15']),
	np.mean(client_all_mt['20'])
]

client_all_as_means = [
	np.mean(client_all_as['1']),
	np.mean(client_all_as['5']),
	np.mean(client_all_as['10']),
	np.mean(client_all_as['15']),
	np.mean(client_all_as['20'])
]

# Plot the results
plt.figure(1)

#data = np.vstack([client_all_mt['1'], client_all_as['1']]).T
x = [1,5,10,15,20]
width = 1/1.5

data = client_all_as_means
plt.bar(x, data, width, color="red", alpha=0.5, label="Asynchronous")

x = [1+width,5+width,10+width,15+width,20+width]
data = client_all_mt_means
plt.bar(x, data, width, color="blue", alpha=0.5, label="Multithreaded")
plt.legend(loc='best')
plt.title("Time to Serve All Client's Sessions on Client-Side")
plt.ylabel("Time (ns)")
plt.xlabel("# of Simultaneous Client Threads")

plt.show()
plt.clf()




