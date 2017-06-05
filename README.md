

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

- fs/indoe.c

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

GPS info of regular files is updated whenever they are created or modified(When i_mtime (modified time) value is modified). Therefore `file_update_time` function calls `set_gps_location` function.

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
```

The GPS-related operations are defined to inode_operation.

- fs/namei.c

accuracy check 

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



### 5. Result



## Lessons

1. We got to know more about inode. We found that inode_info is a wrap around inode, and this relationship allows the linux inode to be designed to make it easier to access the inode structure for each file system.
2. We learned how to handle endian-ness in the kernel. lexx_to_cpu, and cpu_to_lexx unifies big and little endian according to each cpu.

## Team Members

* Park Yoonjong
* Lee Seunggyu
* Lee Nayoung
