/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     light_cwrgb_app.c
* @brief    This file provides demo code for the operation of dimmable light.
* @details
* @author   hector_huang
* @date     2018-11-16
* @version  v1.0
*********************************************************************************************************
*/
#include <math.h>
#include "light_cwrgb_app.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_tim.h"
#include "light_config.h"
#include <trace.h>

static light_t light_cwrgb[] =
{
    /** cold */
    //{P4_3, timer_pwm2, TIM2, 0xffff, 0xffff},
    {P2_7, timer_pwm2, TIM2, 0xffff, 0xffff},
#if DEMO_BOARD
    /** warm: use blue channel */
    {P4_0, timer_pwm6, TIM6, 0xffff, 0xffff},
#else
    /** warm: set to actual pin*/
    //{P1_2, timer_pwm3, TIM3, 0xffff, 0xffff},
    {P2_6, timer_pwm6, TIM6, 0xffff, 0xffff},
#endif
    /** red */
    {P4_1, timer_pwm4, TIM4, 0xffff, 0xffff},
    /** green */
    {P4_2, timer_pwm5, TIM5, 0xffff, 0xffff},
    /** blue */
     {P4_0, timer_pwm6, TIM6, 0xffff, 0xffff},
};

static light_hsl_t light_hsl;
static light_ctl_t light_ctl;

static float hue_2_rgb(float v1, float v2, float h)
{
    if (h < 0)
    {
        h += 1;
    }
    else if (h > 1)
    {
        h -= 1;
    }
    if (6 * h < 1)
    {
        return v1 + (v2 - v1) * 6 * h;
    }
    else if (2 * h < 1)
    {
        return v2;
    }
    else if (3 * h < 2)
    {
        return v1 + (v2 - v1) * (4 - 6 * h);
    }
    else
    {
        return v1;
    }
}

light_rgb_t hsl_2_rgb(light_hsl_t hsl)
{
    light_rgb_t rgb = {0, 0, 0};
    if (0 == hsl.saturation)
    {
        rgb.red = rgb.green = rgb.blue = hsl.lightness;
    }
    else
    {
        float h, s, l, v1, v2;
        h = (float)hsl.hue / 65536;
        s = (float)hsl.saturation / 65535;
        l = (float)hsl.lightness / 65535;
        if (l < 0.5f)
        {
            v2 = l * (1 + s);
        }
        else
        {
            v2 = (l + s) - (s * l);
        }
        v1 = 2 * l - v2;
        rgb.red = ceil(65535 * hue_2_rgb(v1, v2, h + 1.0 / 3));
        rgb.green = ceil(65535 * hue_2_rgb(v1, v2, h));
        rgb.blue = ceil(65535 * hue_2_rgb(v1, v2, h - 1.0 / 3));
    }

    return rgb;
}

light_hsl_t rgb_2_hsl(light_rgb_t rgb)
{
    light_hsl_t hsl = {0, 0, 0};
    uint16_t max, min;
    max = MAX(rgb.red, rgb.green);
    max = MAX(max, rgb.blue);
    min = MIN(rgb.red, rgb.green);
    min = MIN(min, rgb.blue);
    if (max == min)
    {
        hsl.hue = 0;
    }
    else if (max == rgb.red) //!< red: 0 degree
    {
        if (rgb.green >= rgb.blue)
        {
            hsl.hue = (65536 / 6) * (rgb.green - rgb.blue) / (max - min);
        }
        else
        {
            uint32_t tmp = 65536 - (65536 / 6) * (rgb.blue - rgb.green) / (max - min);
            if (tmp == 65536)
            {
                hsl.hue = 0;
            }
            else
            {
                hsl.hue = tmp;
            }
        }
    }
    else if (max == rgb.green) //!< green: 120 degree
    {
        hsl.hue = 65536 / 3 + (65536 / 6) * (rgb.blue - rgb.red) / (max - min);
    }
    else //!< blue: 240 degree
    {
        hsl.hue = 65536 * 2 / 3 + (65536 / 6) * (rgb.red - rgb.green) / (max - min);
    }
    hsl.lightness = (max + min) >> 1;
    if (hsl.lightness == 0 || max == min)
    {
        hsl.saturation = 0;
    }
    else if (hsl.lightness <= 32767)
    {
        hsl.saturation = ceil(65535 * (double)(max - min) / (max + min));
    }
    else
    {
        hsl.saturation = ceil(65535 * (double)(max - min) / (2 * 65535 - max - min));
    }

    return hsl;
}

void light_cwrgb_driver_init(void)
{
    for (uint8_t i = 0; i < sizeof(light_cwrgb) / sizeof(light_t); ++i)
    {
        light_pin_config(&light_cwrgb[i]);
    }
}

void light_cwrgb_enter_dlps(void)
{
    for (uint8_t i = 0; i < sizeof(light_cwrgb) / sizeof(light_t); ++i)
    {
        light_pin_dlps_config(&light_cwrgb[i]);
    }
}

void light_set_cold_lightness(uint16_t lightness)
{
  
	//DBG_DIRECT("===light_set_cold_lightness:%d",lightness);
	
    light_set_lightness(&light_cwrgb[0], lightness); //cool
    light_ctl.lightness = lightness;
}

void light_lighten_cold(uint16_t lightness)
{
    light_lighten(&light_cwrgb[0], lightness);
    light_ctl.lightness = lightness;
}

void light_set_warm_lightness(uint16_t lightness)
{
	//DBG_DIRECT("===light_set_warm_lightness:%d",lightness);
	
    light_set_lightness(&light_cwrgb[1], lightness); //warm
    light_ctl.temperature = lightness;
}

void light_lighten_warm(uint16_t lightness)
{
    light_lighten(&light_cwrgb[1], lightness);
    light_ctl.temperature = lightness;
}

void light_set_cw_lightness(light_cw_t lightness)
{
	//DBG_DIRECT("===light_set_cw_lightness");
	//DBG_DIRECT("===lightness.cold:%d",lightness.cold);
	//DBG_DIRECT("===lightness.warm:%d",lightness.warm);
	
    light_set_lightness(&light_cwrgb[0], lightness.cold);
    light_set_lightness(&light_cwrgb[1], lightness.warm);
	
    light_ctl.lightness = lightness.cold;
    light_ctl.temperature = lightness.warm;
}

void light_lighten_cw(light_cw_t lightness)
{
    
    light_lighten(&light_cwrgb[0], lightness.cold);
    light_lighten(&light_cwrgb[1], lightness.warm);
    light_ctl.lightness = lightness.cold;
    light_ctl.temperature = lightness.warm;
}

light_cw_t light_get_cw_lightness(void)
{
    
    light_cw_t cw = {light_cwrgb[0].lightness, light_cwrgb[1].lightness};
    return cw;
}

void light_set_red_lightness(uint16_t lightness)
{
   
    light_set_lightness(&light_cwrgb[2], lightness);
    light_rgb_t rgb = {light_cwrgb[2].lightness, light_cwrgb[3].lightness, light_cwrgb[4].lightness};
    light_hsl = rgb_2_hsl(rgb);
}

void light_lighten_red(uint16_t lightness)
{
    
    light_lighten(&light_cwrgb[2], lightness);
    light_rgb_t rgb = {light_cwrgb[2].lightness, light_cwrgb[3].lightness, light_cwrgb[4].lightness};
    light_hsl = rgb_2_hsl(rgb);
}

void light_set_green_lightness(uint16_t lightness)
{
    
    light_set_lightness(&light_cwrgb[3], lightness);
    light_rgb_t rgb = {light_cwrgb[2].lightness, light_cwrgb[3].lightness, light_cwrgb[4].lightness};
    light_hsl = rgb_2_hsl(rgb);
}

void light_lighten_green(uint16_t lightness)
{
    
    light_lighten(&light_cwrgb[3], lightness);
    light_rgb_t rgb = {light_cwrgb[2].lightness, light_cwrgb[3].lightness, light_cwrgb[4].lightness};
    light_hsl = rgb_2_hsl(rgb);
}

void light_set_blue_lightness(uint16_t lightness)
{
    
    light_set_lightness(&light_cwrgb[4], lightness);
    light_rgb_t rgb = {light_cwrgb[2].lightness, light_cwrgb[3].lightness, light_cwrgb[4].lightness};
    light_hsl = rgb_2_hsl(rgb);
}

void light_lighten_blue(uint16_t lightness)
{
   
    light_lighten(&light_cwrgb[4], lightness);
    light_rgb_t rgb = {light_cwrgb[2].lightness, light_cwrgb[3].lightness, light_cwrgb[4].lightness};
    light_hsl = rgb_2_hsl(rgb);
}

void light_set_rgb_lightness(light_rgb_t lightness)
{

   
    light_set_lightness(&light_cwrgb[2], lightness.red);
    light_set_lightness(&light_cwrgb[3], lightness.green);
    light_set_lightness(&light_cwrgb[4], lightness.blue);
    light_rgb_t rgb = {light_cwrgb[2].lightness, light_cwrgb[3].lightness, light_cwrgb[4].lightness};
    light_hsl = rgb_2_hsl(rgb);
}

void light_lighten_rgb(light_rgb_t lightness)
{
    
    light_lighten(&light_cwrgb[2], lightness.red);
    light_lighten(&light_cwrgb[3], lightness.green);
    light_lighten(&light_cwrgb[4], lightness.blue);
    light_rgb_t rgb = {light_cwrgb[2].lightness, light_cwrgb[3].lightness, light_cwrgb[4].lightness};
    light_hsl = rgb_2_hsl(rgb);
}

light_rgb_t light_get_rgb_lightness(void)
{
    
    light_rgb_t rgb = {light_cwrgb[2].lightness, light_cwrgb[3].lightness, light_cwrgb[4].lightness};
    return rgb;
}

void light_cw_turn_off(void)
{
    //DBG_DIRECT("===light_cw_turn_off");
    light_lighten(&light_cwrgb[0], 0);
    light_lighten(&light_cwrgb[1], 0);
}

void light_cw_turn_on(void)
{
	//DBG_DIRECT("===light_cw_turn_on");
    if ((0 == light_cwrgb[0].lightness_last) &&
        (0 == light_cwrgb[1].lightness_last))

    {
        light_set_lightness(&light_cwrgb[0], 65535);
        light_set_lightness(&light_cwrgb[1], 65535);
    }
    else
    {
        light_lighten(&light_cwrgb[0], light_cwrgb[0].lightness_last);
        light_lighten(&light_cwrgb[1], light_cwrgb[1].lightness_last);
    }
}

void light_rgb_turn_off(void)
{
    
    light_lighten(&light_cwrgb[2], 0);
    light_lighten(&light_cwrgb[3], 0);
    light_lighten(&light_cwrgb[4], 0);
}

void light_rgb_turn_on(void)
{
    
    if ((0 == light_cwrgb[2].lightness_last) &&
        (0 == light_cwrgb[3].lightness_last) &&
        (0 == light_cwrgb[4].lightness_last))

    {
        light_set_lightness(&light_cwrgb[2], 65535);
        light_set_lightness(&light_cwrgb[3], 65535);
        light_set_lightness(&light_cwrgb[4], 65535);
    }
    else
    {
        light_lighten(&light_cwrgb[2], light_cwrgb[2].lightness_last);
        light_lighten(&light_cwrgb[3], light_cwrgb[3].lightness_last);
        light_lighten(&light_cwrgb[4], light_cwrgb[4].lightness_last);
    }
}

void light_cwrgb_turn_off(void)
{
    
    for (uint8_t i = 0; i < sizeof(light_cwrgb) / sizeof(light_t); ++i)
    {
        light_lighten(&light_cwrgb[i], 0);
    }
}

void light_cwrgb_turn_on(void)
{
  
    if ((0 == light_cwrgb[0].lightness_last) &&
        (0 == light_cwrgb[1].lightness_last) &&
        (0 == light_cwrgb[2].lightness_last) &&
        (0 == light_cwrgb[3].lightness_last) &&
        (0 == light_cwrgb[4].lightness_last))

    {
        light_set_lightness(&light_cwrgb[0], 65535);
        light_set_lightness(&light_cwrgb[1], 65535);
        light_set_lightness(&light_cwrgb[2], 65535);
        light_set_lightness(&light_cwrgb[3], 65535);
        light_set_lightness(&light_cwrgb[4], 65535);
    }
    else
    {
        light_lighten(&light_cwrgb[0], light_cwrgb[0].lightness_last);
        light_lighten(&light_cwrgb[1], light_cwrgb[1].lightness_last);
        light_lighten(&light_cwrgb[2], light_cwrgb[2].lightness_last);
        light_lighten(&light_cwrgb[3], light_cwrgb[3].lightness_last);
        light_lighten(&light_cwrgb[4], light_cwrgb[4].lightness_last);
    }
}

void light_blink_cold(uint32_t hz_numerator, uint32_t hz_denominator, uint8_t duty)
{
    light_blink_infinite(&light_cwrgb[0], hz_numerator, hz_denominator, duty);
}

void light_blink_warm(uint32_t hz_numerator, uint32_t hz_denominator, uint8_t duty)
{
    light_blink_infinite(&light_cwrgb[1], hz_numerator, hz_denominator, duty);
}

void light_blink_red(uint32_t hz_numerator, uint32_t hz_denominator, uint8_t duty)
{
    light_blink_infinite(&light_cwrgb[2], hz_numerator, hz_denominator, duty);
}

void light_blink_green(uint32_t hz_numerator, uint32_t hz_denominator, uint8_t duty)
{
    light_blink_infinite(&light_cwrgb[3], hz_numerator, hz_denominator, duty);
}

void light_blink_blue(uint32_t hz_numerator, uint32_t hz_denominator, uint8_t duty)
{
    light_blink_infinite(&light_cwrgb[4], hz_numerator, hz_denominator, duty);
}

void light_set_ctl(light_ctl_t ctl)
{
	float cool_lightness = 0;
	float warm_lightness = 0;
	
    //DBG_DIRECT("===enter light_set_ctl lightness");
	//DBG_DIRECT("===light_cwrgb[0] ctl.lightness:%d",ctl.lightness);
	//DBG_DIRECT("===light_cwrgb[1] ctl.lightness:%d",ctl.lightness);

	//DBG_DIRECT("===light_cwrgb[0] light_ctl.lightness:%d",light_ctl.lightness);
	//DBG_DIRECT("===light_cwrgb[1] light_ctl.temperature:%d",light_ctl.temperature);

	//DBG_DIRECT("===light_ctl.cool_color_value:%d",light_ctl.cool_color_value);
	//DBG_DIRECT("===light_ctl.warm_color_value:%d",light_ctl.warm_color_value);

	if ((light_ctl.cool_color_value == 0) && (light_ctl.warm_color_value == 0)) //第一次烧录完成，只调整亮度后进入
	{
	
#if 1
			ctl.lightness_present = 65535 / 65535.0;
			ctl.lightness_present_cool = 800 / 65535.0;

			cool_lightness = ctl.lightness * ctl.lightness_present_cool;
			warm_lightness = ctl.lightness * ctl.lightness_present;

			ctl.cool_lightness = (uint16_t)cool_lightness;
			ctl.warm_lightness = (uint16_t)warm_lightness;

			light_set_lightness(&light_cwrgb[0], ctl.cool_lightness);	//cool 
			light_set_lightness(&light_cwrgb[1], ctl.warm_lightness);	//warm
			light_ctl = ctl;
#endif

#if 0
			light_set_lightness(&light_cwrgb[0], ctl.lightness);	//cool 
			light_set_lightness(&light_cwrgb[1], ctl.lightness);	//warm
			light_ctl = ctl;
#endif
	}
	else  //第一次烧录完成，调整色温后进入
	{
		ctl.lightness_present = ctl.lightness / 65535.0;

		//DBG_DIRECT("===ctl.lightness_present :%f",ctl.lightness_present);

		cool_lightness = light_ctl.cool_color_value * ctl.lightness_present;
		warm_lightness = light_ctl.warm_color_value * ctl.lightness_present;

		//DBG_DIRECT("===cool_lightness:%f",cool_lightness);
		//DBG_DIRECT("===warm_lightness:%f",warm_lightness);

		ctl.cool_lightness = (uint16_t)cool_lightness;
		ctl.warm_lightness = (uint16_t)warm_lightness;

		//DBG_DIRECT("===ctl.cool_lightness:%d",ctl.cool_lightness);
		//DBG_DIRECT("===ctl.warm_lightness:%d",ctl.warm_lightness);


#if 1
		light_set_lightness(&light_cwrgb[0], ctl.cool_lightness);	//cool 
		light_set_lightness(&light_cwrgb[1], ctl.warm_lightness);	//warm
		light_ctl = ctl;
#endif


	}




}

void light_set_ctl_tempreture(light_ctl_t ctl)
{
	float temperature_temp = 0;
	//uint16_t color_value = 0;
	
	uint16_t color_uint = 0;
	uint16_t color_max = 20000;
	uint16_t color_min = 800;
	
    //DBG_DIRECT("===enter light_set_ctl_tempreture");
	//DBG_DIRECT("===light_cwrgb[0] ctl.temperature:%d",ctl.temperature);
	//DBG_DIRECT("===light_cwrgb[1] ctl.temperature:%d",ctl.temperature);

	//DBG_DIRECT("===light_ctl.lightness_present:%f",light_ctl.lightness_present);
	
	//color 800-20000
	//color 0-65535
	color_uint = color_max - color_min;
	temperature_temp = ctl.temperature * (65535.0/color_uint);
	//color_value = (uint16_t)temperature_temp;	
	ctl.color_value = (uint16_t)temperature_temp;	
	
	//DBG_DIRECT("===temperature_temp:%f",temperature_temp);
	//DBG_DIRECT("===ctl.color_value:%d",ctl.color_value);	

	if (ctl.temperature == 800)
  	{
  	    //DBG_DIRECT("===enter light_set_ctl_tempreture 800");
  	    ctl.cool_color_value = 800;
  	    ctl.warm_color_value = 65535;
		light_set_lightness(&light_cwrgb[0], ctl.cool_color_value);
    	light_set_lightness(&light_cwrgb[1], ctl.warm_color_value);
	}
	else if (ctl.temperature == 10400)
	{ 
		//DBG_DIRECT("===enter light_set_ctl_tempreture 10400");
		ctl.cool_color_value = 32768;
  	    ctl.warm_color_value = 32768;
		light_set_lightness(&light_cwrgb[0], ctl.cool_color_value);  //cool
		light_set_lightness(&light_cwrgb[1], ctl.warm_color_value);  //warm
	}
	else if(ctl.temperature == 20000)
	{
	    //DBG_DIRECT("===enter light_set_ctl_tempreture 20000");
	   	ctl.cool_color_value = 65535;
  	    ctl.warm_color_value = 800;
		light_set_lightness(&light_cwrgb[0], ctl.cool_color_value);  //cool
		light_set_lightness(&light_cwrgb[1], ctl.warm_color_value);  //warm
	}
	else
	{
	    //DBG_DIRECT("===enter light_set_ctl_tempreture myself define");
	   	ctl.cool_color_value = ctl.color_value;
  	    ctl.warm_color_value = 65535 - ctl.color_value;
        light_set_lightness(&light_cwrgb[0], ctl.cool_color_value);  //cool
        light_set_lightness(&light_cwrgb[1], ctl.warm_color_value);  //warm
	}

	light_ctl = ctl;	
		
}

light_ctl_t light_get_ctl(void)
{
    return light_ctl;
}

void light_set_hsl(light_hsl_t hsl)
{
    light_rgb_t rgb = hsl_2_rgb(hsl);
    light_hsl = hsl;
    light_set_lightness(&light_cwrgb[2], rgb.red);
    light_set_lightness(&light_cwrgb[3], rgb.green);
    light_set_lightness(&light_cwrgb[4], rgb.blue);
}

light_hsl_t light_get_hsl(void)
{
    return light_hsl;
}

light_t *light_get_cold(void)
{
    return &light_cwrgb[0];
}

light_t *light_get_warm(void)
{
    return &light_cwrgb[1];
}

light_t *light_get_red(void)
{
    return &light_cwrgb[2];
}

light_t *light_get_green(void)
{
    return &light_cwrgb[3];
}

light_t *light_get_blue(void)
{
    return &light_cwrgb[4];
}

void light_set_cwrgb(const uint16_t *cwrgb)
{

#if LIGHT_TYPE == LIGHT_LIGHTNESS
    light_set_cold_lightness(cwrgb[0]);
#elif LIGHT_TYPE == LIGHT_CW
    light_cw_t cw = {cwrgb[0], cwrgb[1]};
    light_set_cw_lightness(cw);
#elif LIGHT_TYPE == LIGHT_RGB
    light_rgb_t rgb = {cwrgb[2], cwrgb[3], cwrgb[4]};
    light_set_rgb_lightness(rgb);
#else
    light_set_cold_lightness(cwrgb[0]);
    light_set_warm_lightness(cwrgb[1]);
    light_set_red_lightness(cwrgb[2]);
    light_set_green_lightness(cwrgb[3]);
    light_set_blue_lightness(cwrgb[4]);
#endif
}

void light_get_cwrgb(uint8_t *cwrgb)
{
    cwrgb[0] = light_get_cold()->lightness / 65535.0 * 255;
    cwrgb[1] = light_get_warm()->lightness / 65535.0 * 255;
    cwrgb[2] = light_get_red()->lightness / 65535.0 * 255;
    cwrgb[3] = light_get_green()->lightness / 65535.0 * 255;
    cwrgb[4] = light_get_blue()->lightness / 65535.0 * 255;
}


