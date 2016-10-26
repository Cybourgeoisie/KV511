import json, sys
from random import randint
def main():
	commandList = []
	if len(sys.argv) != 3:
		print "Usage: python jsonGen.py <# requests> <output filename>"
		sys.exit() 
	for i in range(int(sys.argv[1])):
		d = {}
		d['TYPE'] = "GET" if randint(0,1) else "POST"
		d['KEY'] = randint(0,100)
		d['VALUE'] = randint(0,100)
		commandList.append(d)
	with open(sys.argv[2], "w") as f:
		f.write("%s" % json.dumps(commandList, indent=4))
	return
if __name__ == "__main__":
	main()
