#! /bin/sh

if [ $1 == 'clean' ] && [ $2 == 'helloworld' ];then
	rm -f testfile

	cd ./examples/helloworld
	make clean
	cd ../..
elif [ $1 == 'clean' ] && [ $2 == 'libunvmfs' ];then
	rm -f unvmfs_map
	
	cd ./src
	make clean
	cd ../
elif [ $1 == 'clean' ] && [ $2 == 'all' ];then
	rm -f testfile

	cd ./examples/helloworld
	make clean
	cd ../..
	
	rm -f unvmfs_map
	
	cd ./src
	make clean
	cd ../
elif [ $1 == 'make' ] && [ $2 == 'helloworld' ];then
	cd ./examples/helloworld
	make
	cd ../..
elif [ $1 == 'make' ] && [ $2 == 'libunvmfs' ];then
	cd ./src
	make
	cd ../
elif [ $1 == 'make' ] && [ $2 == 'all' ];then
	cd ./src
	make
	cd ../
	
	cd ./examples/helloworld
	make
	cd ../..
else
	echo "please input correct params"
fi

