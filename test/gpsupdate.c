#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define SYSCALL_SET_GPS_LOCATION 380
#define SYSCALL_GET_GPS_LOCATION 381

struct gps_location {
	int lat_integer;
	int lat_fractional;
	int lng_integer;
	int lng_fractional;
	int accuracy;
};

int main(int argc, char **argv)
{
	struct gps_location loc;
	double latitude, longitude;
	int accuracy;

	int lat_integer, lat_fractional;
	int lng_integer, lng_fractional;

	if (argc != 4){ 
		printf("put 3 parameters!\n");
		return -1;
	}

	sscanf(argv[1], "%lf", &latitude);
	sscanf(argv[2], "%lf", &longitude);
	sscanf(argv[3], "%d", &accuracy);

	lat_integer = (latitude>=0) ? (int) latitude : (int) latitude-1;
	lat_fractional = (int) ((latitude - lat_integer) * 1000000); 
	lng_integer = (longitude>=0) ? (int) longitude : (int) longitude-1;
	lng_fractional = (int) ((longitude - lng_integer) * 1000000);

	memcpy(&loc.lat_integer, &lat_integer, sizeof(int));
	memcpy(&loc.lat_fractional, &lat_fractional, sizeof(int));
	memcpy(&loc.lng_integer, &lng_integer, sizeof(int));
	memcpy(&loc.lng_fractional, &lng_fractional, sizeof(int));
	memcpy(&loc.accuracy, &accuracy, sizeof(int));

	printf("input latitude: %d + (%d * 10^-6)\n", loc.lat_integer, loc.lat_fractional);
	printf("input longitude: %d + (%d * 10^-6)\n", loc.lng_integer, loc.lng_fractional);
	printf("input accuracy: %d\n", loc.accuracy);

	if(syscall(SYSCALL_SET_GPS_LOCATION, &loc)!=0){
		printf("SYSCALL_SET_GPS_LOCATION ERROR!\n");
	}
	printf("SYSCALL_SET_GPS_LOCATION success!\n");
	return 0;
}
