#ifndef __SCHEDTUNE_H__
#define __SCHEDTUNE_H__

int schedtune_boost(struct p3_power_module *p3, long long boost_time);
void schedtune_power_init(struct p3_power_module *p3);

#endif /* __SCHEDTUNE_H__ */