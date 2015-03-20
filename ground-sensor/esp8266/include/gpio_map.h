#ifndef GPIO_MAP_H__
#define GPIO_MAP_H__

#include "pin_map.h"

#define USER_GPIO_IS_VALID(gpio) (gpio_pin[gpio] == 0xff ? 0 : 1)
#define USER_GPIO_TO_PIN_MUX(gpio) (pin_mux[gpio_pin[gpio]])
#define USER_GPIO_TO_PIN_FUNC(gpio) (pin_func[gpio_pin[gpio]])

#endif
