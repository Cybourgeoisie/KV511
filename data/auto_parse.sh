# Handle all client threads first
for a in `ls -1 ./results/client/`
do
	cd ./results/client/$a;
	echo $a;
	#/usr/bin/python ../../../analyzeCT.py ./
	cd ../../../;
done

# Then handle all server threads
for a in `ls -1 ./results/server/`
do
	cd ./results/server/$a;
	echo $a;
	#/usr/bin/python ../../../analyzeSMain.py ./
	/usr/bin/python ../../../analyzeST.py ./
	cd ../../../;
done
