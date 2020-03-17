/**
 * @file MPU9250_Defs.h
 * @brief MPU9250 Defines.
 * 
 * This header file contains some useful definitions
 * for the MPU9250 library.
 * @author Davide Marzorati
 * @date 17 March, 2020
*/

#ifndef __MPU9250_DEFS_H
    #define __MPU9250_DEFS_H
    
    #include "cytypes.h"
    
    /**
    *   @brief Error message returned in case of success.
    */
    #define MPU9250_OK 0
    
    /**
    *   @brief Error message returned in case of I2C communication error.
    */
    #define MPU9250_I2C_ERR 1
    
    /**
    *   @brief Error message returned in case of device not found.
    */
    #define MPU9250_DEV_NOT_FOUND_ERR 2 
    
    /**
    *   @brief Unknown error.
    */
    #define MPU9250_UNKNOWN_ERR 3
    
#endif
/* [] END OF FILE */
