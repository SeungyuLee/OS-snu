#ifndef GPS_H_
#define GPS_H_

struct gps_location {
	int lat_integer;
	int lat_fractional;
	int lng_integer;
	int lng_fractional;
	int accuracy;
};

struct gps_location get_gps_location(void);
/*
struct gps_kernel {
	double latitude; // is double ok?
	double longitude;
	int accuracy;
};
*/

#endif
