all:
	cd ./3rd && make clean && make
	cd ./ide && cmake ../src && make

clean:
	cd ./3rd && make clean
	cd ./ide && make clean
