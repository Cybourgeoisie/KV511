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
server_socket_mt  = {'1':[],'5':[],'10':[],'15':[],'20':[]}
server_message_mt = {'1':[],'5':[],'10':[],'15':[],'20':[]}
server_socket_as  = {'1':[],'5':[],'10':[],'15':[],'20':[]}
server_message_as = {'1':[],'5':[],'10':[],'15':[],'20':[]}

# For all possible folders within the results folder
for folder in os.listdir('./' + results_dir):
	
	# Gather all of the output
	client_file = './' + results_dir + '/' + folder + '/outputST.csv'

	# Get the current number from the folder name
	folder_items = folder.split('_')
	num = str(folder_items[-1])

	if os.path.exists(client_file):
		with open(client_file) as csvfile:
	
			# Read the data as a CSV
			client_data = csv.reader(csvfile, delimiter=',')
			for row in client_data:
				for key, el in enumerate(row):
					# analyzeCT and analyzeST
						# 0th element is file. Ignore for now.
						# 1st element is total time elapsed.
						# 2+K elements are session times (client) or message times (server)
					if key > 1:
						if "_mt_t1" in folder:
							server_message_mt['1'].append(float(el))
						elif "_as_t1" in folder:
							server_message_as['1'].append(float(el))
						elif "_mt_" in folder:
							server_message_mt[num].append(float(el))
						else:
							server_message_as[num].append(float(el))

	# Gather all of the output
	client_file = './' + results_dir + '/' + folder + '/outputSMain.csv'

	if os.path.exists(client_file):
		with open(client_file) as csvfile:
	
			# Read the data as a CSV
			client_data = csv.reader(csvfile, delimiter=',')
			for row in client_data:
				for el in row:
					# analyzeSMain
						# All elements are averages for socket times
					if "_mt_t1" in folder:
						server_socket_mt['1'].append(float(el))
					elif "_as_t1" in folder:
						server_socket_as['1'].append(float(el))
					elif "_mt_" in folder:
						server_socket_mt[num].append(float(el))
					else:
						server_socket_as[num].append(float(el))

server_message_mt_means = [
	np.mean(server_message_mt['1']),
	np.mean(server_message_mt['5']),
	np.mean(server_message_mt['10']),
	np.mean(server_message_mt['15']),
	np.mean(server_message_mt['20'])
]

server_message_as_means = [
	np.mean(server_message_as['1']),
	np.mean(server_message_as['5']),
	np.mean(server_message_as['10']),
	np.mean(server_message_as['15']),
	np.mean(server_message_as['20'])
]

print server_message_as_means


# Plot the results
plt.figure(1)

#data = np.vstack([server_message_mt['1'], server_message_as['1']]).T
x = [1,5,10,15,20]
width = 1/1.5

data = server_message_as_means
plt.bar(x, data, width, color="red", alpha=0.5, label="Asynchronous")

x = [1+width,5+width,10+width,15+width,20+width]
data = server_message_mt_means
plt.bar(x, data, width, color="blue", alpha=0.5, label="Multithreaded")
plt.legend(loc='best')
plt.title("Time to Serve a Message on the Server-Side")
plt.ylabel("Time (ns)")
plt.xlabel("# of Simultaneous Client Threads")

plt.show()
plt.clf()


server_socket_mt_means = [
	np.mean(server_socket_mt['1']),
	np.mean(server_socket_mt['5']),
	np.mean(server_socket_mt['10']),
	np.mean(server_socket_mt['15']),
	np.mean(server_socket_mt['20'])
]

server_socket_as_means = [
	np.mean(server_socket_as['1']),
	np.mean(server_socket_as['5']),
	np.mean(server_socket_as['10']),
	np.mean(server_socket_as['15']),
	np.mean(server_socket_as['20'])
]

# Plot the results
plt.figure(1)

#data = np.vstack([server_socket_mt['1'], server_socket_as['1']]).T
x = [1,5,10,15,20]
width = 1/1.5

data = server_socket_as_means
plt.bar(x, data, width, color="red", alpha=0.5, label="Asynchronous")

x = [1+width,5+width,10+width,15+width,20+width]
data = server_socket_mt_means
plt.bar(x, data, width, color="blue", alpha=0.5, label="Multithreaded")
plt.legend(loc='best')
plt.title("Time to Serve a Full Socket on the Server-Side")
plt.ylabel("Time (ns)")
plt.xlabel("# of Simultaneous Client Threads")

plt.show()
plt.clf()




