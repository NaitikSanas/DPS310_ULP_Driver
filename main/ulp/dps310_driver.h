#include <stdint.h>
#include "../dps310_defs.h"

void     dps310_read8     (uint8_t *data_out, uint32_t reg_addr);
void     dps310_write8    (uint8_t data, uint8_t reg_addr);
void     set_device_address(uint8_t address);

float calc_temp(int32_t raw);
uint8_t  check_sensor_ready();
void configure_Temp(uint8_t rate, uint8_t os, uint8_t temp_ext);
int32_t get_temprature_raw();
float calc_temp(int32_t raw);
void     configure_Prs     (uint8_t rate, uint8_t os);
int32_t get_pressure_raw  ();

uint8_t  get_sensor_mode   ();
void     set_sensor_mode   (uint8_t mode);
uint8_t  calib_ready       ();
uint8_t  temp_ready        ();
void     get_coefficients  (void);
uint8_t  get_prod_id       ();
void dps310_reset();
uint8_t prs_ready();
