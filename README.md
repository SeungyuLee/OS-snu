Project 1
===================

## Summary 

The goal of this assignment is implementing a new kernel syncronization primitive which provides reader-writer lock based on device rotation in Linux. First, we wrote new system call to use user-space daemon given from TA. Second, we implemented rotation-based reader-writer locks. Finally, we wrote Selector and Trial for testing.

## Impementation

### 1. set_rotation System Call


```
...
atomic_rotation = ATOMIC_INIT(0);
...
void set_rotation(int degree)
{
	atomic_set(&rotation, degree);
}
...
```

set_rotation() function is implemented in rotation.c . It is called In set_rotation System call to set degree of device to given value. 

### 2. Reader-Writer Lock


#### Policy

* Reader Lock

* Writer Lock

### 3. Test Program

* selector.c

* trial.c

## Lessons

## Team Members

* Park Yoonjong
* Lee Seunggyu
* Lee Nayoung
