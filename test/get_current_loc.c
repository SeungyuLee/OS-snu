#include <linux/gps.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define SYS_GET_CURRENT_LOCATION 382


int main(void){
	struct gps_location loc;

	syscall(SYS_GET_CURRENT_LOCATION, &loc);

	double latitude = (double) loc.lat_integer + (double) loc.lat_fractional * 0.000001;
	double longitude = (double) loc.lng_integer + (double) loc.lng_fractional * 0.000001;

	printf("latitude : %lf\n", latitude);
	printf("longitude : %lf\n", longitude);
	printf("accuracy : %d\n", loc.accuracy);
	printf("Google map : http://www.google.co.kr/maps/@%lf,%lf\n", latitude, longitude);

	return 0;
}
