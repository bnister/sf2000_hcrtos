#ifndef __LINUX_PERCPU_H
#define __LINUX_PERCPU_H

# define __this_cpu_add(pcp, val)	(pcp) += (val)
# define __this_cpu_read(pcp)	(pcp)
# define __this_cpu_inc(pcp)		__this_cpu_add((pcp), 1)
# define __this_cpu_write(pcp, val)	(pcp) = (val)
# define __this_cpu_sub(pcp, val)	__this_cpu_add((pcp), -(typeof(pcp))(val))
# define __this_cpu_dec(pcp)		__this_cpu_sub((pcp), 1)

#endif /* __LINUX_PERCPU_H */
