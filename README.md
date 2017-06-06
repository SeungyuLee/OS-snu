

Project 4
===================

## Summary 

The goal of this project is to develop a new kernel-level mechanism for embedding location information into ext2 file system metadata and use it for access control. We did :

1. tracking device location and adding set gps_location system call
2. adding GPS-related operation to inode for the **ext2** file system. 
3. user-level testing

## Implementation

### 1. High-level design

- include/linux/gps.h

```c
struct gps_location {
	int lat_integer;
	int lat_fractional;
	int lng_integer;
	int lng_fractional;
	int accuracy;
};
```
  `struct gps_location` for information of device gps location is added.


- arch/arm/kernel/gps.c

```c
static struct gps_location dev_location = {
	.lat_integer = 0,
	.lat_fractional = 0,
	.lng_integer = 0,
	.lng_fractional = 0,
	.accuracy = 0,
};
static DEFINE_RWLOCK(dev_location_lock);
```
  The global variable `dev_location` to hold the location of device and Read-Write Lock is declared. `dev_location` needs to be initialized in the begining.

```c
asmlinkage int sys_set_gps_location(struct gps_location __user *loc)
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
```
In `sys_set_gps_location` systemcall, location information from user space is stored in `dev_location`.

```c
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

	if(copy_to_user(loc, &k_loc, sizeof(struct gps_location))){
		kfree(k_pathname);
		printk("copy to user failed\n");
		return -EFAULT;
	}

	kfree(k_pathname);
	return 0;
}
```
In `sys_get_gps_location` systemcall, use the `kern_path` function to get the file's inode that matches the given filepath. Then the gps_location is filled in `get_gps_location` function.

```c
struct gps_location get_gps_location(void)
{
	struct	gps_location device_location;
	read_lock(&dev_location_lock);
	device_location = dev_location;
	read_unlock(&dev_location_lock);
	return device_location;
}
```
Then, gps_location is sent to userspace using `copy_to_user` function.

- fs/ext2/inode.c

```c
...
struct inode *ext2_iget (struct super_block *sb, unsigned long ino)
{
	...
	ei->i_lat_integer = le32_to_cpu(raw_inode->i_lat_integer);
	ei->i_lat_fractional = le32_to_cpu(raw_inode->i_lat_fractional);
	ei->i_lng_integer = le32_to_cpu(raw_inode->i_lng_integer);
	ei->i_lng_fractional = le32_to_cpu(raw_inode->i_lng_fractional);
	ei->i_accuracy = le32_to_cpu(raw_inode->i_accuracy);
	...
}

...

static int __ext2_write_inode(struct inode *inode, int do_sync)
{
	...
	raw_inode->i_lat_integer = cpu_to_le32(ei->i_lat_integer);
	raw_inode->i_lat_fractional = cpu_to_le32(ei->i_lat_fractional);
	raw_inode->i_lng_integer = cpu_to_le32(ei->i_lng_integer);
	raw_inode->i_lng_fractional = cpu_to_le32(ei->i_lng_fractional);
	raw_inode->i_accuracy = cpu_to_le32(ei->i_accuracy);
	...
}

...

int ext2_set_gps_location(struct inode *inode)
{
	struct gps_location cur_loc = get_gps_location();
	struct ext2_inode_info *inode_info = EXT2_I(inode);
	
	inode_info->i_lat_integer = *((__u32 *) &cur_loc.lat_integer);
	inode_info->i_lat_fractional = *((__u32 *) &cur_loc.lat_fractional);
	inode_info->i_lng_integer = *((__u32 *) &cur_loc.lng_integer);
	inode_info->i_lng_fractional = *((__u32 *) &cur_loc.lng_fractional);
	inode_info->i_accuracy = *((__u32 *) &cur_loc.accuracy);

	return 0;
}

int ext2_get_gps_location(struct inode *inode, struct gps_location *loc)
{
	struct ext2_inode_info *inode_info = EXT2_I(inode);

	loc->lat_integer = *((int *)&inode_info->i_lat_integer);
	loc->lat_fractional = *((int *)&inode_info->i_lat_fractional);
	loc->lng_integer = *((int *)&inode_info->i_lng_integer);
	loc->lng_fractional = *((int *)&inode_info->i_lng_fractional);
	loc->accuracy = *((int *)&inode_info->i_accuracy);

	return 0;
}
```

ext2_set_gps_location: Takes the inode as an argument and puts information corresponding to gps_location in the corresponding inode.
ext2_get_gps_location: Takes the inode and struct gps_location as arguments and puts the information corresponding to gps_location in the corresponding inode into the struct gps_location.



- fs/ext2/ext2.h

```c
struct ext2_inode {
...	
	/* gps */
	__le32 i_lat_integer;
  	__le32 i_lat_fractional;
 	__le32 i_lng_integer;
 	__le32 i_lng_fractional;
 	__le32 i_accuracy;
};

struct ext2_inode_info {
...
	/* gps */
 	__u32	i_lat_integer;
 	__u32	i_lat_fractional;
 	__u32	i_lng_integer;
 	__u32	i_lng_fractional;
	__u32	i_accuracy;
 
 	spinlock_t gps_lock;
 };		  
```
5 fields for gps location is added in two struct, inode in the memory and inode on the disk.

- fs/ext2/file.c

```c
const struct inode_operations ext2_file_inode_operations = {
...
	.set_gps_location = ext2_set_gps_location,
	.get_gps_location = ext2_get_gps_location,
};
```

- include/linux/fs.h

```c
struct inode_operations {
	...
	int (*set_gps_location)(struct inode *);
	int (*get_gps_location)(struct inode *, struct gps_location *);
	...
};
```

The GPS-related operations are defined to inode_operation.

- fs/namei.c

```c
long long safety_div(long long x,long long y);
long long apSin(long long x);
long long apCos(long long x);
long long apPow(long long x, int num);
long long apArcTan(long long x);
long long apSqrt(long long x);
long long smallDis(long long lat1, long long lng1, long long lat2, long long lng2);
int getDistance(long long lat1,long long lng1, long long lat2, long long lng2);

int gps_permissionCheck(struct inode *inode);

static inline int do_inode_permission(struct inode *inode, int mask)
{
	...
	if(gps_permissionCheck(inode) == -EACCES) {
		return -EACCES;
	}
	...
}
```

Calculation functions and a permission check functions ard added for location-based file access. More details on assumptions for calculations can be found below.


< The process calling set_gps_location when a File is created/modified >

1) fs/ext2/namei.c

```c
static int ext2_create (struct inode * dir, struct dentry * dentry, umode_t mode, bool excl)
{
	...
	if(inode->i_op->set_gps_location!=NULL){
		printk(KERN_DEBUG "ext2_set_gps_location is called in ext2_create");
		inode->i_op->set_gps_location(inode);
	}
	...
}
```

In `ext2_create` gps_location is set when the file is created.

2) fs/indoe.c

```c
int file_update_time(struct file *file)
{
	...
	if(inode->i_op->set_gps_location){
		printk(KERN_EMERG "set_gps_location(inode) call by file_update_time\n");
		inode->i_op->set_gps_location(inode);	
	}
	...
}
```

In `update_time` function When i_mtime changes, that is, When File is modified, gps_location is set.

3) fs/read_write.c

```c
ssize_t vfs_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	...
	if(inode->i_op->set_gps_location){	
			printk(KERN_DEBUG "set_gps_location by vfs_write\n");
			inode->i_op->set_gps_location(inode);
	}
	...
```

In `vfs_write` function gps_location is also set if the update_time function is not called (ex: when overwriting via echo)


- test/gpsudate.c

sys_get_gps_location system call is called to set the device location with the location information input in user space.

- test/file_loc.c

sys_get_gps_location system call is called to display file or directory location information in user space.

- e2fsprogs-1.43.4/lib/ext2fs/ext2_fs.h

```c
struct ext2_inode {
...
	/* gps */
	__u32 i_lat_integer;
	__u32 i_lat_fractional;
	__u32 i_lng_integer;
	__u32 i_lng_fractional;
	__u32 i_accuracy;
	
...
};

struct ext2_inode_large {
...
/* gps */
	__u32 i_lat_integer;
	__u32 i_lat_fractional;
	__u32 i_lng_integer;
	__u32 i_lng_fractional;
	__u32 i_accuracy;
...
};
```
`struct ext2_inode` and `struct ext2_inode_large` is modifyed to make mke2fs use our modified ext2.


### 4. Location-based file access

* Distance calaulation method

	- Sin approximation
	
	```
	if (x < 0)
	    sin = 1.27323954 * x + .405284735 * x * x;
	else
	    sin = 1.27323954 * x - 0.405284735 * x * x;
	```
		
	- Cos approximation
	
	Sin + pi/2
	
	- Tan approximation
	
	```
	if (x > 5)
		pi / 2
	else if (1.5 <= x && x <= 5) 
		0.008746 * x^3 - 0.11491073 * x^2 + 0.55473370 * x + 0.38484160
	else if (1.5 > x) 
		-0.27216489 * x^2 + 1.06084777 * x - 0.00131597
	```
	
	Assuming that the earth is a 6371-km-long sphere, the distance is calculated using the formula for calculating the length of the arc in the coordinates. At this time, the ArcTan approximation formula becomes smaller than 0, which is caused by an error at a very short distance. Assuming a flat surface at this time, we assume 111.321 km per latitude, and the distance per 1 degree of latitude assumes different values depending on the latitude value.

	1) 0 to 30 degrees: 107.4km (based on 15 degrees)
	2) 30 to 60 degrees: 78.63 km (based on 45 degrees)
	3) 60 to 90 degrees: 28.7 km (75 degrees)

	After assuming this, we obtained the distance using Pythagoras approximation. If both the longitude and latitude differ by more than 0.5 degrees, use the formula to find the length of the arc. In the opposite case, it is assumed that it is a plane and then it is calculated.
	

### 5. Result

File structure of proj4.fs created of the form:

```
|-- proj4
|      -- 301.txt
|      -- 302.txt
|      -- eiffel_tower.txt
|      -- times_square.txt
```

The following is the output that we used to test.

	./file_loc proj4/301.txt
	latitude : 37.450141
	longitude : 126.952550
	accuracy : 50

	./file_loc proj4/302.txt
	latitude : 37.448279
	longitude : 126.952550
	accuracy : 50

	./file_loc proj4/eiffel_tower.txt
	latitude : 48.858384
	longitude : 2.294503
	accuracy : 100

	./file_loc proj4/times_square.txt
	latitude : 40.758862
	longitude : -73.985100
	accuracy : 100

More detailed tests can be found in the demo video :

[![IMAGE ALT TEXT](http://img.youtube.com/vi/https://youtu.be/Hzpif05dVF0/0.jpg)](http://www.youtube.com/watch?v=https://youtu.be/Hzpif05dVF0 "Demo Video")


## Lessons

1. We got to know more about inode. We found that inode_info is a wrap around inode, and this relationship allows the linux inode to be designed to make it easier to access the inode structure for each file system.
2. We learned how to handle endian-ness in the kernel. lexx_to_cpu, and cpu_to_lexx unifies big and little endian according to each cpu.

## Team Members

* Park Yoonjong
* Lee Seunggyu
* Lee Nayoung
