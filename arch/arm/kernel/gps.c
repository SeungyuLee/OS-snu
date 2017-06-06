#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/gps.h>
#include <linux/namei.h>
#include <linux/fs.h>

static struct gps_location dev_location = {
	.lat_integer = 0,
	.lat_fractional = 0,
	.lng_integer = 0,
	.lng_fractional = 0,
	.accuracy = 0,
}; /* needs to be initialized in the beginning */

static DEFINE_RWLOCK(dev_location_lock);

struct gps_location get_gps_location(void)
{
	struct	gps_location device_location;
	read_lock(&dev_location_lock);
	device_location = dev_location;
	read_unlock(&dev_location_lock);
	return device_location;
}

asmlinkage int sys_set_gps_location(struct gps_location __user *loc)
{
	struct gps_location k_loc;

	if (loc == NULL)
		return -EINVAL;

	if (copy_from_user(&k_loc, loc, sizeof(struct gps_location)))
		return -EFAULT;

	if (k_loc.lat_integer < -90 || k_loc.lat_integer > 90)
		return -EINVAL;
	
	if (k_loc.lng_integer < -180 || k_loc.lng_integer > 180)
		return -EINVAL;
	
	if (k_loc.lat_fractional < 0 || k_loc.lat_fractional > 999999)
		return -EINVAL;
	
	if (k_loc.lng_fractional < 0 || k_loc.lat_fractional > 999999)
		return -EINVAL;

	if (k_loc.accuracy < 0)
		return -EINVAL;

	write_lock(&dev_location_lock);
	dev_location.lat_integer = k_loc.lat_integer; 
	dev_location.lat_fractional = k_loc.lat_fractional;
	dev_location.lng_integer = k_loc.lng_integer;
	dev_location.lng_fractional = k_loc.lng_fractional;
	dev_location.accuracy = k_loc.accuracy;
	write_unlock(&dev_location_lock);

	return 0;
}

asmlinkage int sys_get_gps_location(const char __user *pathname, struct gps_location __user *loc)
{
	/* this syscall has to be reviewed again */
	char *k_pathname;
	struct gps_location k_loc;
	struct inode *inode;
	struct path path;
	int result;

	if(pathname == NULL || loc == NULL){
		printk("pathname is null or loc is null error\n");
		return -EINVAL;
	}

	k_pathname = kcalloc(PATH_MAX, sizeof(char), GFP_KERNEL);
	
	if(k_pathname == NULL){
		printk("No memory for k_pathname\n");
		return -ENOMEM;
	}
	
	result = strncpy_from_user(k_pathname, pathname, PATH_MAX);
	
	if(result < 0){
		kfree(k_pathname);
		printk("strncpy_from_user error\n");
		return -EFAULT;
	}

	if(kern_path(k_pathname, LOOKUP_FOLLOW, &path)){
		kfree(k_pathname);
		printk("cannot find the path\n");
		return -EINVAL;
	}

	inode = path.dentry->d_inode;

	if(!(S_IRUSR & inode->i_mode)){
		kfree(k_pathname);
		printk("Access error\n");
		return -EACCES;
	}


	if(inode->i_op->get_gps_location==NULL){
		kfree(k_pathname);
		printk("No get_gps_location inode operation\n");
		return -ENODEV;
	}
	else
		inode->i_op->get_gps_location(inode, &k_loc);

	if(gps_permissionCheck(inode)==-EACCES)
		return -EACCES;

	if(copy_to_user(loc, &k_loc, sizeof(struct gps_location))){
		kfree(k_pathname);
		printk("copy to user failed\n");
		return -EFAULT;
	}

	kfree(k_pathname);
	return 0;
}

asmlinkage int sys_get_current_location(struct gps_location __user *loc)
{
	read_lock(&dev_location_lock);
	if(copy_to_user(loc, &(dev_location), sizeof(*loc))!=0) {
		printk("copy_to_user failed\n");
		return -1;
	}
	read_unlock(&dev_location_lock);

	return 0;
}
