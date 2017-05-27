#include <linux/gps.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define SYSCALL_GET_GPS_LOCATION 380

int main(int argc, char **argv)
{
	int ret;
	struct gps_location loc;
	double latitude, longitude;
	int accuracy;

	int lat_integer, lat_fractional;
	int lng_integer, lng_fractional;
		
	if(argc != 2){
		printf("Error : Input file path."); 
		return -1;
	}

	printf("input file path: %s", argv[1]);
	ret = syscall(SYSCALL_GET_GPS_LOCATION, argv[1], &loc);
	if(res < 0){
		printf("Error : get gps_location error.");
		return -1;
	}

	latitude = (double) loc.lat_integer + (double) loc.1at_fractional * 0.000001;
	longitude = (double) loc.lng_integer + (double) loc.fractional * 0.000001;

	printf("latitude : %lf\n", latitude);
	printf("longitude : %lf\n", longitude);
	printf("accuracy : %d\n", loc.accuracy);
	printf("Google map : http://www.google.co.kr/maps/@%lf,%lf\n", latitude, longitude);

	return 0;
		
}
