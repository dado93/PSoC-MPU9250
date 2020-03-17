/* ========================================
 * This source file contains the functions
 * used to interface with the MPU9250.
 * 
 * Functions declarations are present in the
 * MPU9250.h header file.
 *
 * ========================================
*/

/* ========= Includes ========= */
#include "MPU9250.h"
#include "MPU9250_RegMap.h"
#include "MPU9250_I2C.h"
#include "math.h"
#include "UART_1.h"
#include "stdio.h"

/* ========= MACROS ========= */
#ifndef MPU9250_ACC_FS_MASK 
    #define MPU9250_ACC_FS_MASK 0x18 // This mask is used for acc full scale bits
#endif

#ifndef MPU9250_GYRO_FS_MASK 
    #define MPU9250_GYRO_FS_MASK 0x18 // This mask is used for gyro full scale bits
#endif

#ifndef MPU9250_SLEEP_MASK 
    #define MPU9250_SLEEP_MASK 0x40
#endif

#ifndef MPU9250_G
    #define MPU9250_G 9.807f
#endif

/* ========= VARIABLES ========= */
float acc_scale = 0;    // Accelerometer scaling factor
float gyro_scale = 0;   // Gyroscope scaling factor

void MPU9250_Start(void) {
    // This function starts the MPU9250.
    
    // Check if the I2C component has already been started,
    // otherwise start it.
    if (!I2C_MPU9250_Master_initVar) {
        I2C_MPU9250_Master_Start();
        CyDelay(10);
    }
    
    // Wake up MPU9250
    MPU9250_WakeUp();
    
    // Set up default accelerometer full scale range
    MPU9250_SetAccFS(MPU9250_Acc_FS_2g);
    
    // Set up defaul gyroscope full scale range
    MPU9250_SetGyroFS(MPU9250_Gyro_FS_250);
    
    // Set up sample rate divider
    MPU9250_SetSampleRateDivider(4); // From 1kHz to 200 Hz sampling
    
    // Set up gyroscope, temperature digital low pass filter
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_CONFIG_REG, 0x03);
    
    // Set up accelerometer digital low pass filter
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_ACCEL_CONFIG_2_REG, 0x03);
    
    // Configure interrupt
    MPU9250_SetInterruptActiveHigh();
    MPU9250_SetInterruptPushPull();
    MPU9250_HeldInterruptPin();
    MPU9250_ClearInterruptStatusReg();
    MPU9250_EnableI2CBypass();
    
    MPU9250_EnableRawDataInterrupt();
    MPU9250_DisableFifoOverflowInterrupt();
    MPU9250_DisableWomInterrupt();
    MPU9250_DisableFsyncInterrupt();    
}

void MPU9250_Sleep(void) {
    // This function sleeps the MPU9250 by entering sleep mode.
    
    // Set sleep bit in power management 1 register.
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_PWR_MGMT_1_REG,  
        ( MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_PWR_MGMT_1_REG) | MPU9250_SLEEP_MASK));
}

void MPU9250_WakeUp(void) {
    // This function wakes up the MPU9250 exiting sleep mode.
    
    // Clear sleep bit in power management 1 register.
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_PWR_MGMT_1_REG,  
        ( MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_PWR_MGMT_1_REG) & ~MPU9250_SLEEP_MASK));
}

uint8_t MPU9250_IsConnected(void) {
    // Checks if the MPU9250 is present on the I2C bus
    uint8_t err = I2C_MPU9250_Master_MasterSendStart(MPU9250_I2C_ADDRESS, 0);
    I2C_MPU9250_Master_MasterSendStop();
    if (err > 0)
        return 0;
    // Then also check if the value contained in the who am i register is the expected one
    return MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_WHO_AM_I_REG) == MPU9250_WHO_AM_I;
}

uint8_t MPU9250_ReadWhoAmI(void) {
    // Reads the who am i register and return the value
    return MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_WHO_AM_I_REG);   
}

uint8_t MPU9250_ReadMagWhoAmI(void) {
    // Reads the who am i register of the magnetometer
    return MPU9250_I2C_Read(AK8963_I2C_ADDRESS, 0x00);
}

void MPU9250_ReadAcc(int16_t* acc) {
    // We can read 6 consecutive bytes since the accelerometer registers are in order
    
    uint8_t temp[6];  // Temp variable to store the data
    
    // Read data via I2C
    MPU9250_I2C_ReadMulti(MPU9250_I2C_ADDRESS, MPU9250_ACCEL_XOUT_H_REG, temp, 6);
    acc[0] = (temp[0] << 8) | (temp[1] & 0xFF);
    acc[1] = (temp[2] << 8) | (temp[3] & 0xFF);
    acc[2] = (temp[4] << 8) | (temp[5] & 0xFF);
}

void MPU9250_ReadAccRaw(uint8_t* acc) {
    // We can read 6 consecutive bytes since the accelerometer registers are in order
    
    // Read data via I2C
    MPU9250_I2C_ReadMulti(MPU9250_I2C_ADDRESS, MPU9250_ACCEL_XOUT_H_REG, acc, 6);
}

void MPU9250_ReadGyro(int16_t* gyro) {
    // We can read 6 consecutive bytes since the gyroscope registers are in order
    
    uint8_t temp[6];  // Temp variable to store the data
    
    // Read data via I2C
    MPU9250_I2C_ReadMulti(MPU9250_I2C_ADDRESS, MPU9250_GYRO_XOUT_H_REG, temp, 6);
    gyro[0] = (temp[0] << 8) | (temp[1] & 0xFF);
    gyro[1] = (temp[2] << 8) | (temp[3] & 0xFF);
    gyro[2] = (temp[4] << 8) | (temp[5] & 0xFF);
}

void MPU9250_ReadGyroRaw(uint8_t* gyro) {
    // We can read 6 consecutive bytes since the gyroscope registers are in order
    
    // Read data via I2C
    MPU9250_I2C_ReadMulti(MPU9250_I2C_ADDRESS, MPU9250_GYRO_XOUT_H_REG, gyro, 6);
}

void MPU9250_ReadAccGyro(int16_t* acc, int16_t* gyro) {
    // We can read 14 consecutive bytes since the accelerometer and
    // gyroscope registers are in order
    
    uint8_t temp[14];  // Temp variable to store the data
    
    // Read data via I2C
    MPU9250_I2C_ReadMulti(MPU9250_I2C_ADDRESS, MPU9250_ACCEL_XOUT_H_REG, temp, 14);
    acc[0]  = (temp[0] << 8) | (temp[1] & 0xFF);
    acc[1]  = (temp[2] << 8) | (temp[3] & 0xFF);
    acc[2]  = (temp[4] << 8) | (temp[5] & 0xFF);
    gyro[0] = (temp[8] << 8) | (temp[9] & 0xFF);
    gyro[1] = (temp[10] << 8) | (temp[11] & 0xFF);
    gyro[2] = (temp[12] << 8) | (temp[13] & 0xFF);
}

void MPU9250_ReadAccGyroRaw(uint8_t* data) {

    // Read data via I2C
    MPU9250_I2C_ReadMulti(MPU9250_I2C_ADDRESS, MPU9250_ACCEL_XOUT_H_REG, data, 6);
    MPU9250_I2C_ReadMulti(MPU9250_I2C_ADDRESS, MPU9250_GYRO_XOUT_H_REG, data + 6, 6);
}

void MPU9250_ReadMag(int16_t* mag) {
    
    uint8_t temp[6];
    // Get RAW data
   MPU9250_ReadMagRaw(temp);
    
    mag[0] = (temp[0] << 8) | (temp[1] & 0xFF);
    mag[1] = (temp[2] << 8) | (temp[3] & 0xFF);
    mag[2] = (temp[4] << 8) | (temp[5] & 0xFF);
}

void MPU9250_ReadMagRaw(uint8_t* mag) {
       
    // Read data via I2C
    MPU9250_I2C_ReadMulti(AK8963_I2C_ADDRESS, MPU9250_MAG_XOUT_H_REG, mag, 6);
}

void MPU9250_ReadSelfTestGyro(int16_t* self_test_gyro) {
    uint8_t temp[6];
    
    MPU9250_I2C_ReadMulti(MPU9250_I2C_ADDRESS, MPU9250_SELF_TEST_X_GYRO_REG, temp, 6);
    self_test_gyro[0] = ( temp[0] << 8) | ( temp[1] & 0xFF);
    self_test_gyro[1] = ( temp[1] << 8) | ( temp[3] & 0xFF);
    self_test_gyro[2] = ( temp[2] << 8) | ( temp[5] & 0xFF);
}

void MPU9250_ReadSelfTestAcc(int16_t* self_test_acc) {
    uint8_t temp[6];
    
    MPU9250_I2C_ReadMulti(MPU9250_I2C_ADDRESS, MPU9250_SELF_TEST_X_ACCEL_REG, temp, 6);
    self_test_acc[0] = ( temp[0] << 8) | ( temp[1] & 0xFF);
    self_test_acc[1] = ( temp[1] << 8) | ( temp[3] & 0xFF);
    self_test_acc[2] = ( temp[2] << 8) | ( temp[5] & 0xFF);
}

void MPU9250_SelfTest(float* deviation) {
    // Perform self test of accelerometer and gyroscope according to the
    // procedure described in the application note MPU-9250 Accelerometer, Gyroscope and
    // Compass Self-Test Implementation.
    
    MPU9250_Acc_FS Old_Acc_FS;    // Old value of accelerometer full scale range
    MPU9250_Gyro_FS Old_Gyro_FS;  // Old value of accelerometer full scale range
    int16_t ST_Acc[3] = {0,0,0};  // Accelerometer values with self test enabled
    int16_t Acc[3] = {0,0,0};     // Accelerometer average valus without self test enabled
    int16_t ST_Gyro[3] = {0,0,0}; // Gyroscope values with self test enabled
    int16_t Gyro[3] = {0,0,0};    // Gyroscope values without self test enabled
    int16_t ST_Response[6];       // Self test response on acc and gyro 3 axis
    
    // Get current accelerometer full scale range
    Old_Acc_FS = MPU9250_GetAccFS();
    // Get current gyroscope full scale range
    Old_Gyro_FS = MPU9250_GetGyroFS();
    
    // Set gyroscope full scale range to 250dps
    MPU9250_SetGyroFS(MPU9250_Gyro_FS_250);
    // Set accelerometer full scale range to 2g
    MPU9250_SetAccFS(MPU9250_Acc_FS_2g);
    
    // Write 0 to sample rate divider register -- no additional divider
    MPU9250_SetSampleRateDivider(0x00);
    
    // Set gyroscope and accelerometer DLPF configuration to 1kHz Fs, 92 Hz bandwidth
    // First set up configuration register so that DLPF CFG is set to 2
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_CONFIG_REG, 0x02);
    // Then write 00 in FChoice_b of GYRO config register
    uint8_t temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_GYRO_CONFIG_REG);
    temp &= ~ 0x02; // Clear bits [1:0]
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_GYRO_CONFIG_REG, temp);
    // Set accelerometer DLPF configuration
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_ACCEL_CONFIG_REG, 0x02);
    // Get 200 readings 
    int16_t Acc_Temp[3];
    int16_t Gyro_Temp[3];
    
    for (int reading = 0; reading < 200; reading++) {
        MPU9250_ReadAcc(Acc_Temp);
        Acc[0] += Acc_Temp[0];
        Acc[1] += Acc_Temp[1];
        Acc[2] += Acc_Temp[2];
        MPU9250_ReadGyro(Gyro_Temp);
        Gyro[0] += Gyro_Temp[0];
        Gyro[1] += Gyro_Temp[1];
        Gyro[2] += Gyro_Temp[2];
    }
    // .. and average them
    for (int i = 0; i < 3; i++) {
        Acc[i]  /= 200;
        Gyro[i] /= 200;
    }
    char message[50];
    sprintf(message, "Avg: %5d %5d %5d -- %5d %5d %5d\r\n", Acc[0]*100, Acc[1]*100, Acc[2]*100, Gyro[0]*100, Gyro[1]*100, Gyro[2]*100);
    UART_1_PutString(message);
    // Enable self test gyroscope
    temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_GYRO_CONFIG_REG);
    // Then, set bits [7,6,5]
    temp |= 0b11100000;
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_GYRO_CONFIG_REG, temp);
    // Enable self test accelerometer
    temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_ACCEL_CONFIG_REG);
    // Then, set bits [7,6,5]
    temp |= 0b11100000;
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_ACCEL_CONFIG_REG, temp);
    
    // Wait 20 ms so that everything is stable
    CyDelay(20);
    
    for (int reading = 0; reading < 200; reading++) {
        MPU9250_ReadAcc(Acc_Temp);
        ST_Acc[0] += Acc_Temp[0];
        ST_Acc[1] += Acc_Temp[1];
        ST_Acc[2] += Acc_Temp[2];
        MPU9250_ReadGyro(Gyro_Temp);
        ST_Gyro[0] += Gyro_Temp[0];
        ST_Gyro[1] += Gyro_Temp[1];
        ST_Gyro[2] += Gyro_Temp[2];
    }
    // .. and average them
    for (int i = 0; i < 3; i++) {
        ST_Acc[i]  /= 200;
        ST_Gyro[i] /= 200;
    }
    sprintf(message, "STg: %5d %5d %5d -- %5d %5d %5d\r\n", ST_Acc[0]*100, ST_Acc[1]*100, ST_Acc[2]*100, ST_Gyro[0]*100, ST_Gyro[1]*100, ST_Gyro[2]*100);
    UART_1_PutString(message);
    // Disable self test gyroscope -- Read config register
    temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_GYRO_CONFIG_REG);
    // Clear bits [7,6,5]
    temp &= ~0b11100000;
    // Write new value to the register
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_GYRO_CONFIG_REG, temp);
    // Disbale self test accelerometer -- Read config register
    temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_ACCEL_CONFIG_REG);
    // Clear bits [7,6,5]
    temp &= ~0b11100000;
    // Write new vale to the register
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_ACCEL_CONFIG_REG, temp);
    
    // Wait 20 ms so that everything is stable
    CyDelay(20);
    
    // Restore full scale range values
    MPU9250_SetAccFS(Old_Acc_FS);
    MPU9250_SetGyroFS(Old_Gyro_FS);
    
    // Compute self test responses
    ST_Response[0] = ST_Acc[0] - Acc[0];
    ST_Response[1] = ST_Acc[1] - Acc[1];
    ST_Response[2] = ST_Acc[2] - Acc[2];
    ST_Response[3] = ST_Gyro[0] - Gyro[0];
    ST_Response[4] = ST_Gyro[1] - Gyro[1];
    ST_Response[5] = ST_Gyro[2] - Gyro[2];
    
    // Get values stored in the device for self test
    int16_t ST_AccStored[3];
    int16_t ST_GyroStored[3];
    
    MPU9250_ReadSelfTestAcc(ST_AccStored);
    MPU9250_ReadSelfTestGyro(ST_GyroStored);
    
    // Compute factory trim
    float Factory_Trim[6];
    Factory_Trim[0] = (float) ( ( 2620 / 1 << MPU9250_Acc_FS_2g) * pow( 1.01, ST_AccStored[0] - 1.0) );
    Factory_Trim[1] = (float) ( ( 2620 / 1 << MPU9250_Acc_FS_2g) * pow( 1.01, ST_AccStored[1] - 1.0) );
    Factory_Trim[2] = (float) ( ( 2620 / 1 << MPU9250_Acc_FS_2g) * pow( 1.01, ST_AccStored[2] - 1.0) );
    Factory_Trim[3] = (float) ( ( 2620 / 1 << MPU9250_Acc_FS_2g) * pow( 1.01, ST_GyroStored[0] - 1.0) );
    Factory_Trim[4] = (float) ( ( 2620 / 1 << MPU9250_Acc_FS_2g) * pow( 1.01, ST_GyroStored[1] - 1.0) );
    Factory_Trim[5] = (float) ( ( 2620 / 1 << MPU9250_Acc_FS_2g) * pow( 1.01, ST_GyroStored[2] - 1.0) );
    
    // Compute deviation
    for (int i = 0; i < 3; i++) {
        deviation[i] = 100.0 * ((float) (ST_Response[i] - Acc[i])) / Factory_Trim[i] - 100;
        deviation[i+3] = 100.0 * ((float) (ST_Response[i] - Gyro[i+3])) / Factory_Trim[i] - 100;
    }
}

void MPU9250_SetAccFS(MPU9250_Acc_FS fs) {
    // Write the new full scale value in the acc conf register
   
    // We need to first read the current bits of the register
    uint8_t temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_ACCEL_CONFIG_REG);
    // Then, we clear bits [4:3]
    temp &= ~MPU9250_ACC_FS_MASK;
    // Lastly, we write the new byte to the register
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_ACCEL_CONFIG_REG, temp | ( fs << 3));
    
    // We also need to update the scaling factor
    switch(fs) {
    case MPU9250_Acc_FS_2g:
        acc_scale = MPU9250_G * 2.0f/pow(2,16)/2;
        break;
    case MPU9250_Acc_FS_4g:
        acc_scale = MPU9250_G * 4.0f/pow(2,16)/2;
        break;
    case MPU9250_Acc_FS_8g:
        acc_scale = MPU9250_G * 8.0f/pow(2,16)/2;
        break;
    case MPU9250_Acc_FS_16g:
        acc_scale = MPU9250_G * 16.0f/pow(2,16)/2;
        break;
        
    }
}

MPU9250_Acc_FS MPU9250_GetAccFS(void) {
    // Get the current full scale range of the accelerometer
    
    // First, get all the register bits
    uint8_t temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_ACCEL_CONFIG_REG);
    // Mask all bits expect [4:3]
    temp &= MPU9250_ACC_FS_MASK;
    // Shift them by 3
    temp = temp >> 3;
    return temp;
}

void MPU9250_SetGyroFS(MPU9250_Gyro_FS fs) {
    // Write the new full scale value in the gyro conf register
    
    // We need to first read the current bits of the register
    uint8_t temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_GYRO_CONFIG_REG);
    // Then, we clear bits [4:3]
    temp &= ~MPU9250_GYRO_FS_MASK;
    // Lastly, we write the new byte to the register
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_GYRO_CONFIG_REG, temp | ( fs << 3));
}

MPU9250_Gyro_FS MPU9250_GetGyroFS(void) {
    // Get the current full scale range of the accelerometer
    
    // First, get all the register bits
    uint8_t temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_GYRO_CONFIG_REG);
    // Mask all bits expect [4:3]
    temp &= MPU9250_GYRO_FS_MASK;
    // Shift them by 3
    temp = temp >> 3;
    return temp;
}

void MPU9250_SetSampleRateDivider(uint8_t smplrt) {
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_SMPLRT_DIV_REG, smplrt);
}

void MPU9250_ReadAccelerometerOffset(int16_t *acc_offset) {
    // Get the accelerometer offset values
    uint8_t temp[6] = {'\0'};
    MPU9250_I2C_ReadMulti(MPU9250_I2C_ADDRESS, MPU9250_XA_OFFSET_H_REG, temp, 6);
    acc_offset[0] = (temp[0] << 8) | (temp[1] & 0xFF);
    acc_offset[0] = (temp[2] << 8) | (temp[3] & 0xFF);
    acc_offset[0] = (temp[4] << 8) | (temp[5] & 0xFF);
}

void MPU9250_EnableRawDataInterrupt(void) {
    // Set bit [0] of MPU9250_INT_EN_REG
    // Read current value
    uint8_t temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG);
    // Set bit[0]
    temp |= 0x01;
    // Write new value
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG, temp);
}

void MPU9250_DisableRawDataInterrupt(void) {
    // Clear bit [0] of MPU9250_INT_EN_REG
    // Read current value
    uint8_t temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG);
    // Clear bit[0]
    temp &= ~0x01;
    // Write new value
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG, temp);
}

void MPU9250_EnableFsyncInterrupt(void) {
    // Set bit [3] of MPU9250_INT_EN_REG
    // Read current value
    uint8_t temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG);
    // Set bit[3]
    temp |= 0x08;
    // Write new value
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG, temp);
}

void MPU9250_DisableFsyncInterrupt(void) {
    // Clear bit [3] of MPU9250_INT_EN_REG
    // Read current value
    uint8_t temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG);
    // Clear bit[3]
    temp &= ~0x08;
    // Write new value
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG, temp);
}

void MPU9250_EnableFifoOverflowInterrupt(void) {
    // Set bit [4] of MPU9250_INT_EN_REG
    // Read current value
    uint8_t temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG);
    // Set bit[4]
    temp |= 0x10;
    // Write new value
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG, temp);
}

void MPU9250_DisableFifoOverflowInterrupt(void) {
    // Clear bit [4] of MPU9250_INT_EN_REG
    // Read current value
    uint8_t temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG);
    // Clear bit[4]
    temp &= ~0x10;
    // Write new value
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG, temp);
}

void MPU9250_EnableWomInterrupt(void) {
    // Set bit [6] of MPU9250_INT_EN_REG
    // Read current value
    uint8_t temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG);
    // Set bit[6]
    temp |= 0x40;
    // Write new value
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG, temp);
}

void MPU9250_DisableWomInterrupt(void) {
    // Clear bit [6] of MPU9250_INT_EN_REG
    // Read current value
    uint8_t temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG);
    // Clear bit[6]
    temp &= ~0x40;
    // Write new value
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG, temp);
}

uint8_t MPU9250_ReadInterruptStatus(void) {
    return MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_INT_STATUS_REG);
}

void MPU9250_SetInterruptActiveHigh(void) {
    // Clear bit [7] of MPU9250_INT_PIN_CFG_REG
    // Read current value
    uint8_t temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG);
    // Clear bit[7]
    temp &= ~0x80;
    // Write new value
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG, temp);
}

void MPU9250_SetInterruptActiveLow(void) {
    // Set bit [7] of MPU9250_INT_PIN_CFG_REG
    // Read current value
    uint8_t temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_INT_PIN_CFG_REG);
    // Set bit[7]
    temp |= 0x40;
    // Write new value
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_INT_PIN_CFG_REG, temp);
}

void MPU9250_SetInterruptOpenDrain(void) {
    // Clear bit [6] of MPU9250_INT_PIN_CFG_REG
    // Read current value
    uint8_t temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG);
    // Clear bit[6]
    temp &= ~0x40;
    // Write new value
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG, temp);
}

void MPU9250_SetInterruptPushPull(void) {
    // Set bit [6] of MPU9250_INT_PIN_CFG_REG
    // Read current value
    uint8_t temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_INT_PIN_CFG_REG);
    // Set bit[6]
    temp |= 0x40;
    // Write new value
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_INT_PIN_CFG_REG, temp);
}

void MPU9250_HeldInterruptPin(void) {
    // Set bit[5] of MPU9250_INT_PIN_CFG_REG
    // Read current value
    uint8_t temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_INT_PIN_CFG_REG);
    // Set bit[5]
    temp |= 0x20;
    // Write new value
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_INT_PIN_CFG_REG, temp);
}
    
void MPU9250_InterruptPinPulse(void) {
    // Clear bit [5] of MPU9250_INT_PIN_CFG_REG
    // Read current value
    uint8_t temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG);
    // Clear bit[5]
    temp &= ~0x20;
    // Write new value
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG, temp);   
}

void MPU9250_ClearInterruptAny(void) {
    // Set bit[4] of MPU9250_INT_PIN_CFG_REG
    // Read current value
    uint8_t temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_INT_PIN_CFG_REG);
    // Set bit[4]
    temp |= 0x10;
    // Write new value
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_INT_PIN_CFG_REG, temp);   
}

void MPU9250_ClearInterruptStatusReg(void) {
    // Clear bit [4] of MPU9250_INT_PIN_CFG_REG
    // Read current value
    uint8_t temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG);
    // Clear bit[4]
    temp &= ~0x10;
    // Write new value
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG, temp);      
}

void MPU9250_EnableI2CBypass(void) {
    // Clear bit [5] of MPU9250_USER_CTRL_REG
    // Read current value
    uint8_t temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_USER_CTRL_REG);
    // Clear bit [5]
    temp &= ~0x20;
    // Write new value
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_USER_CTRL_REG, temp);   
    
    
    // Set bit [1] of MPU9250_INT_PIN_CFG_REG
    // Read current value
    temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_INT_PIN_CFG_REG);
    // Set bit[1]
    temp |= 0x02;
    // Write new value
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_INT_PIN_CFG_REG, temp);
}

void MPU9250_DisableI2CBypass(void) {
    // Set bit [5] of MPU9250_INT_ENABLE_REG
    // Read current value
    uint8_t temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_USER_CTRL_REG);
    // Set bit [5]
    temp |= 0x20;
    // Write new value
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_USER_CTRL_REG, temp); 
    
    // Clear bit [1] of MPU9250_INT_PIN_CFG_REG
    // Read current value
    temp = MPU9250_I2C_Read(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG);
    // Clear bit[1]
    temp &= ~0x02;
    // Write new value
    MPU9250_I2C_Write(MPU9250_I2C_ADDRESS, MPU9250_INT_ENABLE_REG, temp);
}

void MPU9250_MAG_Enable(void) {
    
    // 0x00 = MAG off (default)
    // 0x01 = MAG on (14-bit output)
    // 0x02 = Continuous mode 1
    // 0x11 = MAG on AND 16-bit output
    MPU9250_I2C_Write(AK8963_I2C_ADDRESS, MPU9250_MAG_CNTL1_REG, 0x11);
    
    CyDelay(10);
    
}

void MPU9250_MAG_Disable(void) {
    
    MPU9250_I2C_Write(AK8963_I2C_ADDRESS, MPU9250_MAG_CNTL1_REG, 0x00);
    
    CyDelay(10);
    
}
/* [] END OF FILE */
