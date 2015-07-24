
#ifndef DS18B20_H__
#define DS18B20_H__

struct ds18b20_t {
  uint8_t addr[8];
  uint16_t tVal, tFract;
  char tSign;
};

extern bool exec_ds18b20(int gpio, struct ds18b20_t* results, int max_results, int* found_results);

#endif
