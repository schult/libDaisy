#include "stm32h7xx_hal.h"
#include "per_gpio.h"

void	dsy_gpio_init(dsy_gpio *p) 
{
	GPIO_TypeDef *   port;
	GPIO_InitTypeDef ginit;
	switch(p->mode)
	{
		case DSY_GPIO_MODE_INPUT: ginit.Mode = GPIO_MODE_INPUT; break;
		case DSY_GPIO_MODE_OUTPUT_PP: ginit.Mode = GPIO_MODE_OUTPUT_PP; break;
		case DSY_GPIO_MODE_OUTPUT_OD: ginit.Mode = GPIO_MODE_OUTPUT_OD; break;
		default: ginit.Mode = GPIO_MODE_INPUT; break;
	}
	switch(p->pull)
	{
		case DSY_GPIO_NOPULL: ginit.Pull = GPIO_NOPULL; break;
		case DSY_GPIO_PULLUP: ginit.Pull = GPIO_PULLUP; break;
		case DSY_GPIO_PULLDOWN: ginit.Pull = GPIO_PULLDOWN; break;
		default: ginit.Pull = GPIO_NOPULL; break;
	}
	ginit.Speed = GPIO_SPEED_LOW;
	port		= (GPIO_TypeDef*)gpio_hal_port_map[p->pin.port];
	ginit.Pin   = gpio_hal_pin_map[p->pin.pin];
	HAL_GPIO_Init(port, &ginit);
}

void	dsy_gpio_deinit(dsy_gpio *p) 
{
	GPIO_TypeDef *port;
	uint16_t	  pin;
	port = (GPIO_TypeDef *)gpio_hal_port_map[p->pin.port];
	pin  = gpio_hal_pin_map[p->pin.pin];
	HAL_GPIO_DeInit(port, pin);
}

uint8_t dsy_gpio_read(dsy_gpio *p) 
{
	return HAL_GPIO_ReadPin((GPIO_TypeDef*)gpio_hal_port_map[p->pin.port], gpio_hal_pin_map[p->pin.pin]);
}

void dsy_gpio_write(dsy_gpio *p, uint8_t state)
{
	HAL_GPIO_WritePin((GPIO_TypeDef *)gpio_hal_port_map[p->pin.port],
					  gpio_hal_pin_map[p->pin.pin],
					  (GPIO_PinState)(state > 0 ? 1 : 0));
}
void dsy_gpio_toggle(dsy_gpio *p)
{
	HAL_GPIO_TogglePin((GPIO_TypeDef *)gpio_hal_port_map[p->pin.port],
					   gpio_hal_pin_map[p->pin.pin]);
}
