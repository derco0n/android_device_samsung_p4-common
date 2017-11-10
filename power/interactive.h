#ifndef __INTERACTIVE_H__
#define __INTERACTIVE_H__

#define CPUFREQ_INTERACTIVE "/sys/devices/system/cpu/cpufreq/interactive/"

void boost_on(struct p3_power_module *p3, void *data);
void boostpulse(struct p3_power_module *p3);
void boostpulse_init(struct p3_power_module *p3);

#endif /* __INTERACTIVE_H__ */