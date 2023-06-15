/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* ULP RISC-V RTC I2C example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <math.h>
#include "esp_sleep.h"
#include "ulp_riscv.h"
#include "ulp_riscv_i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ulp_main.h"


// For GPIO:
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "soc/rtc_periph.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");
#include "dps310_defs.h"
#include "ulp/dps310_driver.h"
/************************************************
 * ULP utility APIs
 ************************************************/
static void init_ulp_program(void);

/************************************************
 * RTC I2C utility APIs
 ************************************************/
static void init_i2c(void);


static int32_t oversample_scalefactor[] = {524288, 1572864, 3670016, 7864320,
                                           253952, 516096,  1040384, 2088960};

float m_last_temp_scal = 0;

typedef struct data_container
{
    /* data */
    float temprature;
    float pressure;
} dps310_data_t;



dps310_data_t calc_temp_prs(int32_t raw_temp, int32_t raw_prs){
    //get_coefficients();   
    dps310_data_t result; 
    printf("raw value %d\n",raw_temp);
    float temprature  = (float)raw_temp;     
    /* 
        copying variable from ulp mem to local.
        found out compiler bug preventing floating point
        opertation with shared ulp variables directly. 
    */
    int32_t m_c1 = ulp_m_c1;
    int32_t m_c0_half = ulp_m_c0Half;
    float temprature_scaled  = temprature / oversample_scalefactor[ulp_tmp_os];
    temprature = temprature_scaled;
    temprature *= (float)m_c1 ;
    temprature += (float)m_c0_half; 
    result.temprature = temprature;

    printf("RAW pressure %d\n",raw_prs);
    float pressure = (float)raw_prs;
    int32_t c00 = (int32_t)ulp_m_c00;
    int32_t c10 = (int32_t)ulp_m_c10;
    int32_t c01 = (int32_t)ulp_m_c01;
    int32_t c11 = (int32_t)ulp_m_c11;
    int32_t c20 = (int32_t)ulp_m_c20;
    int32_t c21 = (int32_t)ulp_m_c21;
    int32_t c30 = (int32_t)ulp_m_c30;

    float f_c00 = c00;
    float f_c10 = c10;
    float f_c01 = c01;
    float f_c11 = c11;
    float f_c20 = c20;
    float f_c21 = c21;
    float f_c30 = c30;

    printf("%d|%d|%d|%d|%d|%d|%d\n",ulp_m_c00,ulp_m_c10,ulp_m_c01,ulp_m_c11,ulp_m_c20,ulp_m_c21,ulp_m_c30);
    printf("%d|%d|%d|%d|%d|%d|%d\n",c00,c10,c01,c11,c20,c21,c30);
    printf("%x|%x|%x|%x|%x|%x|%x\n",c00,c10,c01,c11,c20,c21,c30);
    printf("TOS : %d | POS : %d\n",ulp_tmp_os,ulp_prs_os);
    pressure = pressure/oversample_scalefactor[ulp_prs_os];
    printf("pressur_scaled %f \n",pressure);
    pressure =   f_c00 + pressure * (f_c10 + pressure * (f_c20 + pressure * f_c30)) +
                 temprature_scaled * (f_c01 + pressure * (f_c11 + pressure * f_c21)); 
    printf("pressure %f\n",pressure);
    result.pressure = pressure/100;
    return result;
}


void app_main(void)
{
   

    // rtc_gpio_init(GPIO_NUM_4);
    // rtc_gpio_set_direction(GPIO_NUM_4, RTC_GPIO_MODE_OUTPUT_ONLY);
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    /* Not a wakeup from ULP
     * Initialize RTC I2C
     * Setup BMP180 sensor
     * Store current temperature and pressure values
     * Load the ULP firmware
     * Go to deep sleep
     */
    if (cause != ESP_SLEEP_WAKEUP_ULP && cause != 11) {
        printf("Not a ULP-RISC V wakeup (cause = %d)\n", cause);

        /* Initialize RTC I2C */
        init_i2c();
        ulp_riscv_i2c_master_set_slave_addr(0x77);

        
        /* Load ULP firmware
         *
         * The ULP is responsible of monitoring the temperature and pressure values
         * periodically. It will wakeup the main CPU if the temperature and pressure
         * values are above a certain threshold.
         */
        init_ulp_program();
    }

    /* ULP RISC-V read and detected a temperature or pressure above the limit */
    if (cause == ESP_SLEEP_WAKEUP_ULP) {
        printf("ULP RISC-V woke up the main CPU\n");

        /* Pause ULP while we are using the RTC I2C from the main CPU */
        ulp_timer_stop();
        ulp_riscv_halt();
        /* Resume ULP and go to deep sleep again */
        ulp_timer_resume();
    } else {
        printf("(cause = %d)\n", cause);
        ulp_timer_stop();
        ulp_riscv_halt();
        vTaskDelay(100);
        ulp_timer_resume();
    }

    dps310_data_t data = calc_temp_prs(ulp_temp_raw,ulp_prs_raw);
    printf("[DATA:] TMP : %f *C | PRS : %f *hPa\n",data.temprature, data.pressure );
    /* Add a delay for everything to the printed before heading in to light sleep */
    vTaskDelay(100);

    /* Go back to sleep, only the ULP RISC-V will run */

    /* RTC peripheral power domain needs to be kept on to keep RTC I2C related configs during sleep */
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

    ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());

    esp_deep_sleep_start();
}

static void init_i2c(void)
{
    /* Configure RTC I2C */
    printf("Initializing RTC I2C ...\n");
    ulp_riscv_i2c_cfg_t i2c_cfg = ULP_RISCV_I2C_DEFAULT_CONFIG();
    printf("SDA PIN %d \n", i2c_cfg.i2c_pin_cfg.sda_io_num);
    printf("SCL PIN %d \n", i2c_cfg.i2c_pin_cfg.scl_io_num);
    ulp_riscv_i2c_master_init(&i2c_cfg);
}

static void init_ulp_program(void)
{
    esp_err_t err = ulp_riscv_load_binary(ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start));
    ESP_ERROR_CHECK(err);

    /* The first argument is the period index, which is not used by the ULP-RISC-V timer
     * The second argument is the period in microseconds, which gives a wakeup time period of: 20ms
     */
    ulp_set_wakeup_period(0, 40000);

    /* Start the program */
    err = ulp_riscv_run();
    ESP_ERROR_CHECK(err);
}
