
void dps310_init(){
    /* set temp/pressure precision */
    uint8_t pm_prc = PM_PRC_x16;
    uint8_t tmp_prc = TEMP_PRC_x16;
    /* runs this once on startup only */
    if(!init_done){     
        set_device_address(DPS310_DEF_ADDR);          //Set Defaul i2c device address for dps310              
        uint8_t tmp = get_prod_id();     //get sensor product id id
        if(tmp!=0x10){
            #if ENABLE_ULP_LOGS
                ulp_riscv_print_str("No Device found \n"); 
            #endif   
        }
        else{ 
            init_done = true; //set this flag to prevent initiaLization on multitple calls
            #if ENABLE_ULP_LOGS
                ulp_riscv_print_str("Product ID: ");
                ulp_riscv_print_hex(tmp);
                ulp_riscv_print_str("\n");
            #endif
            dps310_reset(); //reset sensor registers to default state.
            dps310_read8(&tmp,COEF_SRCE); //get co-efficients source

             while (!calib_ready()) //wait for calibration coefficient data 
             {
                 ulp_riscv_print_str("calib not ready\n");
             }
            get_coefficients(); //fetch co-efficients. 
            
            /* prints co-efficients from buffer */
            #if ENABLE_ULP_LOGS
                ulp_riscv_print_str("---Coefficients---\n");
                for(int i = 0; i < 18; i++){
                ulp_riscv_print_hex(buffer[i]);
                ulp_riscv_print_str("\n");
                }
                ulp_riscv_print_str("------------------");
            #endif
            set_sensor_mode(MEAS_CTRL_IDLE);  //put sensor into standby mode.
            configure_Temp(TEMP_RATE_x2,tmp_prc,1); //configure temprature
            
            /* for debugging purpose checking config_temp register values */

            

            #if ENABLE_ULP_LOGS
                uint8_t tmp_config  = 0;
                dps310_read8(&tmp_config, TMP_CFG); 
                ulp_riscv_print_str("Temp Config");
                ulp_riscv_print_hex(tmp_config);
                ulp_riscv_print_str("\n");
            #endif

            
            configure_Prs(PM_RATE_x2,pm_prc); //configure pressure
            /* for debugging purpose checking config_prs register values */
            #if ENABLE_ULP_LOGS
                uint8_t prs_config  = 0;
                dps310_read8(&prs_config, PRS_CFG); 
                ulp_riscv_print_str("PRS Config");
                ulp_riscv_print_hex(prs_config);
                ulp_riscv_print_str("\n");
            #endif
            
            if(tmp_prc > 3 && pm_prc > 3)dps310_write8(0x0C,CFG_REG);
            else if(tmp_prc > 3 && pm_prc < 3)dps310_write8(0x08,CFG_REG);
            else if(tmp_prc < 3 && pm_prc > 3)dps310_write8(0x04,CFG_REG);
            else if(tmp_prc < 0 && pm_prc < 3)dps310_write8(0x00,CFG_REG);




            set_sensor_mode(MEAS_CTRL_IDLE); //Set Sensor operation mode.
            measure_tmp_prc(); //measure tmp and pressure for once.

            /* 
                from offical library it was recommended to write these
                value to fixe problem in measuring temprature changes.
            */
            dps310_write8(0xA5,0x0E);
            dps310_write8(0x96,0x0F);
            dps310_write8(0x02,0x62);
            dps310_write8(0x00,0x0E);
            dps310_write8(0x00,0x0F);


            measure_tmp_prc(); //another temp & pressure meausurement call.
        }
    }
}