// Seunggyu Lee (03/16/2017)

#ifndef __PRINFO_H__
#define __PRINFO_H__

#include <linux/types.h> // defines pid_t

struct prinfo {
        long state;             /* current state of process */
        pid_t pid;              /* process id */
        pid_t parent_pid;       /* process id of parent */
        pid_t first_child_pid;  /* pid of oldest child */
        pid_t next_sibling_pid; /* pid of younger sibling */
        long uid;               /* user id of process owner */
        char comm[64];          /* name of program executed */
};

#endif /* __PRINFO_H__ */


