#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

long long apSin(long long x) { 
	if (x < -314159265) {
		x += 628318531;
	}
	else if( x > 314159265) {
		x -= 628318531;
	}
	long long second = 40528473;
	second = second * x;
	second /= 100000000;
	second = second * x;
	second /= 100000000;
	if (x < 0) {
		x = 127323954 * x;
		x /= 100000000;
		return (x+second);
	}
	else {
		x = 127323954 * x;
		x /= 100000000;
		return (x - second);
	}	
}

long long apCos(long long x) {
	x += 157079632;
	return apSin(x);
	/*
	if (x < -3141592) {
		x += 6283158;
	}
	else if (x > 3141592) {
		x -= 6283158;
	}
	long long second = 405284;
	second = second * x / 1000000;
	second = second * x / 1000000;
	if ( x < 0) {
		return (1273239 * x / 1000000 + second);
	}
	else {
		return (1273239 * x / 1000000 - second);
	}
	*/
}

long long apPow(long long x, int num) {
	long long res = x;
	int i;
	for(i=1;i<num;i++) {
		res = res * x;
		res /= 100000000;
	}
	return res;
}

long long apArcTan(long long x) {
	long long res;
	if (x > 500000000) return 314159265/2;
	else if (x > 150000000) {
		res = 874060 * apPow(x,3);
		res /= 100000000;
		long long second = 11491073 * apPow(x,2);
		second /= 100000000;
		long long third = 55473370 * x;
		third /= 100000000;
		res = res - second + third + 38484160;
	}else {
		res = -27216489 * apPow(x,2);
		printf("apPow(x,2) : %lld\n",apPow(x,2));
		res /= 100000000;
		printf("res1 : %lld\n",res);
		long long second = 106084777 * x;
		second /= 100000000;
		printf("second: %lld\n",second);
		res = res + second - 131597;
		printf("res2: %lld\n",res);
	}
	return res;
}

long long apSqrt(long long x) {
	long long i;
	for(i=1; ; i++) {
		if (i*i > x) {
			return (i-1) * 10000;
		}
	}
}

long long getDistance(long long lat1,long long lng1, long long lat2, long long lng2) {
	long long radius = 6371;
	long long dLat = (lat2 - lat1) * 3141592;
	dLat /= 180;
	dLat /= 10000;
	printf("dLat: %lld\n", dLat);
	long long dLng = (lng2 - lng1) * 3141592;
	dLng /= 180;
	dLng /= 10000;
	printf("dLng: %lld\n", dLng);
	long long pt1y = lat1 * 3141592;
	pt1y /= 180;
	pt1y /= 10000;
	long long pt2y = lat2 * 3141592;
	pt2y /= 180;
	pt2y /= 10000;

	printf("pt1y: %lld\n", pt1y);
	printf("Cos(pt1y) : %lld\n", apCos(pt1y));
	printf("pt2y: %lld\n", pt2y);
	printf("Cos(pt2y) : %lld\n", apCos(pt2y));
	dLat /= 2;
	dLng /= 2;
	printf("Sin(dLat) : %lld\n",apSin(dLat));
	printf("Sin(dLng) : %lld\n", apSin(dLng));
	long long a = apSin(dLat) * apSin(dLat);
	a /= 100000000;
	long long second = apSin(dLng) * apSin(dLng);
	second /= 100000000;

	printf("a: %lld\n", a);
	printf("second1: %lld\n", second);
	second = second * apCos(pt1y);
	second /= 100000000;
	printf("second2: %lld\n", second);
	second = second * apCos(pt2y);
	second /= 100000000;
	printf("second3: %lld\n", second);
	a = a + second;
	long long b = apSqrt(a) * 100000000;
	printf("apSqrt(a) : %lld\n",apSqrt(a));
	printf("b1 : %lld\n",b);
	b /= apSqrt(100000000-a);
	printf("b2 : %lld\n",b);
	long long c = 2 * apArcTan(b);
	printf("apArcTan(b) : %lld\n",apArcTan(b));

	return radius * c;
}

int main(int argc, char **argv) {
  
  long long lat1,lng1,lat2,lng2;
  
  if (argc != 5){ 
    printf("put 4 parameters!\n");
    return -1;
  }
  
  sscanf(argv[1], "%lld", &lat1);
  sscanf(argv[2], "%lld", &lng1);
  sscanf(argv[3], "%lld", &lat2);
  sscanf(argv[4], "%lld", &lng2);

  printf("x: %lld %lld\n dev : %lld %lld\n dis : %lld\n",lat1,lng1,lat2,lng2,getDistance(lat1,lng1,lat2,lng2));
  
	return 0;
}
