#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/gps.h>

static struct gps_kernel dev_location;
static DEFINE_RWLOCK(dev_location_lock);

asmlinkage int set_gps_location(struct gps_location __user *loc)
{
	struct gps_location k_loc;

	/*
	// error handling on access right violation
	if (current_uid() != 0 && current_euid() != 0)
		return -EACCES;
	*/
	if (loc == NULL)
		return -EINVAL;

	if (copy_from_user(&k_loc, loc, sizeof(struct gps_location)))
		return -EFAULT;

	if (k_loc->lat_fractional < 0 || k_loc->lat_fractional > 999999)
		return -EINVAL;
	
	if (k_loc->lng_fractional < 0 || k_loc->lat_fractional > 999999)
		return -EINVAL;

	if (k_loc->accuracy < 0)
		return -EINVAL;

	write_lock(&dev_location_lock);
	dev_location.latitude = k_loc->lat_integer + k_loc->lat_fractional * 0.000001;
	dev_location.longitude = k_loc->lng_integer + k_loc->lng_fractional * 0.000001;
	dev_location.accuracy = k_lock->accuracy;
	write_unlock(&dev_location_lock);

	return 0;
}

asmlinkage int get_gps_location(const char __user *pathname, struct gps_location __user *loc)
{

	return 0;
}

