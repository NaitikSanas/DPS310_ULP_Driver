#include "dps310_driver.h"
#include "ulp_riscv.h"
#include "ulp_riscv_utils.h"
#include "ulp_riscv_i2c_ulp_core.h"
#include "ulp_riscv_print.h"
#include "ulp_riscv_uart_ulp_core.h"


static int32_t oversample_scalefactor[] = {524288, 1572864, 3670016, 7864320,
                                           253952, 516096,  1040384, 2088960};
/* co-efficents */
int32_t m_c00;
int32_t m_c10;
int32_t m_c01;
int32_t m_c11;
int32_t m_c20;
int32_t m_c21;
int32_t m_c30;
int32_t m_c1;
int32_t m_c0Half;
uint8_t buffer[18];
uint8_t tmp_os, prs_os;

uint32_t temp_scale;

void getTwosComplement(int32_t *raw, uint8_t length)
{
	if (*raw & ((uint32_t)1 << (length - 1)))
	{
		*raw -= (uint32_t)1 << length;
	}
}

static int32_t twosComplement(int32_t val, uint8_t bits) {
  if (val & ((uint32_t)1 << (bits - 1))) {
    val -= (uint32_t)1 << bits;
  }
  return val;
}

void dps310_read8(uint8_t *data_out, uint32_t reg_addr)
{    
    ulp_riscv_i2c_master_set_slave_reg_addr(reg_addr);
    ulp_riscv_i2c_master_read_from_device(data_out, 1);
}

void dps310_write8(uint8_t data, uint8_t reg_addr){
    ulp_riscv_i2c_master_set_slave_reg_addr(reg_addr);
    ulp_riscv_i2c_master_write_to_device(&data, 1);
    ulp_riscv_delay_cycles(ULP_RISCV_CYCLES_PER_MS * 50);
}

int32_t get_temprature_raw(){
    uint8_t byte[3];
    int32_t temp=0;
    // ulp_riscv_i2c_master_set_slave_reg_addr(TMP_B2);
    // ulp_riscv_i2c_master_read_from_device(byte,3);
    dps310_read8(&byte[2],TMP_B2);
    dps310_read8(&byte[1],TMP_B1);
    dps310_read8(&byte[0],TMP_B0);
    
    temp = byte[2] << 16 | byte[1] << 8 | byte[0];
    temp = twosComplement(temp,24);
    return temp;
}

int32_t get_pressure_raw(){
    uint8_t byte[3];
    int32_t psr=0;
    //ulp_riscv_i2c_master_set_slave_reg_addr(PSR_B2);
    //ulp_riscv_i2c_master_read_from_device(byte,3);
    dps310_read8(&byte[2],PSR_B2);
    dps310_read8(&byte[1],PSR_B1);
    dps310_read8(&byte[0],PSR_B0);
    psr = byte[2] << 16 | byte[1] << 8 | byte[0];
    psr = twosComplement(psr,24);
    return psr;
}

void get_coefficients(void){
   ulp_riscv_i2c_master_set_slave_reg_addr(COEF_START);
    ulp_riscv_i2c_master_read_from_device(buffer, 18);
	//compose coefficients from buffer content
	m_c0Half = ((uint32_t)buffer[0] << 4) | (((uint32_t)buffer[1] >> 4) & 0x0F);
	getTwosComplement(&m_c0Half, 12);
	//c0 is only used as c0*0.5, so c0_half is calculated immediately
	m_c0Half = m_c0Half / 2U;

	//now do the same thing for all other coefficients
	m_c1 = (((uint32_t)buffer[1] & 0x0F) << 8) | (uint32_t)buffer[2];
	getTwosComplement(&m_c1, 12);
	m_c00 = ((uint32_t)buffer[3] << 12) | ((uint32_t)buffer[4] << 4) | (((uint32_t)buffer[5] >> 4) & 0x0F);
	getTwosComplement(&m_c00, 20);
	m_c10 = (((uint32_t)buffer[5] & 0x0F) << 16) | ((uint32_t)buffer[6] << 8) | (uint32_t)buffer[7];
	getTwosComplement(&m_c10, 20);

	m_c01 = ((uint32_t)buffer[8] << 8) | (uint32_t)buffer[9];
	getTwosComplement(&m_c01, 16);

	m_c11 = ((uint32_t)buffer[10] << 8) | (uint32_t)buffer[11];
	getTwosComplement(&m_c11, 16);
	m_c20 = ((uint32_t)buffer[12] << 8) | (uint32_t)buffer[13];
	getTwosComplement(&m_c20, 16);
	m_c21 = ((uint32_t)buffer[14] << 8) | (uint32_t)buffer[15];
	getTwosComplement(&m_c21, 16);
	m_c30 = ((uint32_t)buffer[16] << 8) | (uint32_t)buffer[17];
	getTwosComplement(&m_c30, 16);

}

uint8_t check_sensor_ready(){
    uint8_t v = 0;
    dps310_read8(&v,MEAS_CFG);
    v = (v&0x40) >> 6;
    return v;
} 

uint8_t calib_ready(){
    uint8_t v = 0;
    dps310_read8(&v, MEAS_CFG);
    v = v >> 7;
    return v;
}

void dps310_reset(){
    dps310_write8(0x89,RESET);
    uint8_t v = 0;  
     ulp_riscv_delay_cycles(ULP_RISCV_CYCLES_PER_MS * 200);
}


// float calc_temp(int32_t raw){
//     float temprature  = (float)raw;
//     int32_t m_c1      = m_c1;
//     int32_t m_c0_half = m_c0Half;
//     temprature  = temprature / oversample_scalefactor[tmp_os]; 
//     temprature *= (float)m_c1 ;
//     temprature += (float)m_c0_half;
//     return temprature;
// }

uint8_t temp_ready(){
    uint8_t v = 0;
    dps310_read8(&v, MEAS_CFG);
    v = (v&20) >> 5;
    return v;
}

void configure_Temp(uint8_t rate, uint8_t os, uint8_t temp_ext){
    tmp_os = os;
    temp_ext = temp_ext << 7;
    temp_scale = oversample_scalefactor[os];
    temp_ext |= (rate << 4 | os);
    dps310_write8(temp_ext,TMP_CFG);
    
    uint8_t tmp = 0;
    if(os > 3){     
        dps310_read8(&tmp,CFG_REG);
        tmp |= (1 << 3);
        dps310_write8(tmp, CFG_REG);
    }

}

uint8_t prs_ready(){
    uint8_t v = 0;
    dps310_read8(&v, MEAS_CFG);
    v = (v&10) >> 4;
    return v;
}

void configure_Prs(uint8_t rate, uint8_t os){
    uint8_t val = rate << 4 | os;
    dps310_write8(val,PRS_CFG);
    prs_os = os;  
    uint8_t tmp = 0;
    if(os > 3){
        dps310_read8(&tmp,CFG_REG);
        tmp |= (1 << 2);
        dps310_write8(tmp, CFG_REG);
    }

}


void set_device_address(uint8_t address){
    ulp_riscv_i2c_master_set_slave_addr(address);
}

uint8_t get_sensor_mode(){
    uint8_t ret = 0;
    dps310_read8(&ret,MEAS_CFG);
    return ret;
}
void set_sensor_mode(uint8_t mode){
    uint8_t v = get_sensor_mode();
    v = (v&0b111) | mode;
    dps310_write8(v,MEAS_CFG);    
}

uint8_t get_prod_id(){
    uint8_t ret = 0;   
    dps310_read8(&ret,PROD_ID);
    return ret;
}

