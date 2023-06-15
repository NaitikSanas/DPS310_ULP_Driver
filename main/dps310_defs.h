#define DPS310_DEF_ADDR     0x77
#define PSR_B2     0x00
#define PSR_B1     0x01
#define PSR_B0     0x02
    
#define TMP_B2     0x03
#define TMP_B1     0x04
#define TMP_B0     0x05

#define PRS_CFG    0x06
#define TMP_CFG    0x07


#define MEAS_CFG   0x08
#define CFG_REG    0x09

#define INT_STS    0x0A
#define FIFO_STS   0x0B
#define RESET      0x0C

#define PROD_ID    0x0D
#define COEF_START 0x10
#define COEF_SRCE  0x28


/* MEAS CFG Fields */
#define MEAS_CTRL_IDLE        0
#define MEAS_CTRL_PRESSURE    1
#define MEAS_CTRL_TEMPERATURE 2
#define MEAS_CONT_PRESS       5
#define MEAS_CONT_TEMP        6
#define MEAS_CONT_PRESS_TEMP  7

/* TMP rate  */

#define TEMP_RATE_x1   0
#define TEMP_RATE_x2   1 
#define TEMP_RATE_x4   2
#define TEMP_RATE_x8   3
#define TEMP_RATE_x16  4
#define TEMP_RATE_x32  5
#define TEMP_RATE_x64  6
#define TEMP_RATE_x128 7

/*TMP PRC */
#define TEMP_PRC_x1   0
#define TEMP_PRC_x2   1 
#define TEMP_PRC_x4   2
#define TEMP_PRC_x8   3
#define TEMP_PRC_x16  4
#define TEMP_PRC_x32  5
#define TEMP_PRC_x64  6
#define TEMP_PRC_x128 7



/* TMP rate  */

#define PM_RATE_x1   0
#define PM_RATE_x2   1 
#define PM_RATE_x4   2
#define PM_RATE_x8   3
#define PM_RATE_x16  4
#define PM_RATE_x32  5
#define PM_RATE_x64  6
#define PM_RATE_x128 7

/*TMP PRC */
#define PM_PRC_x1   0
#define PM_PRC_x2   1 
#define PM_PRC_x4   2
#define PM_PRC_x8   3
#define PM_PRC_x16  4
#define PM_PRC_x32  5
#define PM_PRC_x64  6
#define PM_PRC_x128 7

