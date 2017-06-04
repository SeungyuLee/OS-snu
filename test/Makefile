all : 
	arm-linux-gnueabi-gcc -I ../include file_loc.c -o file_loc
	arm-linux-gnueabi-gcc -I ../include gpsupdate.c -o gpsupdate
	arm-linux-gnueabi-gcc -I ../include get_current_loc.c -o get_current_loc
clean :
	rm file_loc gpsupdate
