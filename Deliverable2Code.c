//Areeb Ahmed 400583758
#include <stdint.h>
#include "PLL.h"
#include "SysTick.h"
#include "uart.h"
#include "onboardLEDs.h"
#include "tm4c1294ncpdt.h"
#include "VL53L1X_api.h"





#define I2C_MCS_ACK             0x00000008  // Data Acknowledge Enable
#define I2C_MCS_DATACK          0x00000008  // Acknowledge Data
#define I2C_MCS_ADRACK          0x00000004  // Acknowledge Address
#define I2C_MCS_STOP            0x00000004  // Generate STOP
#define I2C_MCS_START           0x00000002  // Generate START
#define I2C_MCS_ERROR           0x00000002  // Error
#define I2C_MCS_RUN             0x00000001  // I2C Master Enable
#define I2C_MCS_BUSY            0x00000001  // I2C Busy
#define I2C_MCR_MFE             0x00000010  // I2C Master Function Enable

#define MAXRETRIES              5           // number of receive attempts before giving up
void I2C_Init(void){
  SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0;           													// activate I2C0
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;          												// activate port B
  while((SYSCTL_PRGPIO_R&0x0002) == 0){};																		// ready?

    GPIO_PORTB_AFSEL_R |= 0x0C;           																	// 3) enable alt funct on PB2,3       0b00001100
    GPIO_PORTB_ODR_R |= 0x08;             																	// 4) enable open drain on PB3 only

    GPIO_PORTB_DEN_R |= 0x0C;             																	// 5) enable digital I/O on PB2,3
//    GPIO_PORTB_AMSEL_R &= ~0x0C;          																// 7) disable analog functionality on PB2,3

                                                                            // 6) configure PB2,3 as I2C
//  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00003300;
  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00002200;    //TED
    I2C0_MCR_R = I2C_MCR_MFE;                      													// 9) master function enable
    I2C0_MTPR_R = 0b0000000000000101000000000111011;                       	// 8) configure for 100 kbps clock (added 8 clocks of glitch suppression ~50ns)
//    I2C0_MTPR_R = 0x3B;                                        						// 8) configure for 100 kbps clock
        
}

//The VL53L1X needs to be reset using XSHUT.  We will use PG0
void PortG_Init(void){
    //Use PortG0
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R6;                // activate clock for Port N
    while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R6) == 0){};    // allow time for clock to stabilize
    GPIO_PORTG_DIR_R |= 0x01;                                        // make PG0 in (HiZ)
  GPIO_PORTG_AFSEL_R &= ~0x01;                                     // disable alt funct on PG0
  GPIO_PORTG_DEN_R |= 0x01;                                        // enable digital I/O on PG0
                                                                                                    // configure PG0 as GPIO
  //GPIO_PORTN_PCTL_R = (GPIO_PORTN_PCTL_R&0xFFFFFF00)+0x00000000;
  GPIO_PORTG_AMSEL_R &= ~0x01;                                     // disable analog functionality on PN0

    return;
}

void PortM_Init(void){
	//Use PortM pins (PM0-PM3) for output
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R11;				// activate clock for Port M
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R11) == 0){};	// allow time for clock to stabilize
	GPIO_PORTM_DIR_R |= 0x0F;        								// configure Port M pins (PM0-PM3) as output
  GPIO_PORTM_AFSEL_R &= ~0x0F;     								// disable alt funct on Port M pins (PM0-PM3)
  GPIO_PORTM_DEN_R |= 0x0F;        								// enable digital I/O on Port M pins (PM0-PM3)
																									// configure Port M as GPIO
  GPIO_PORTM_AMSEL_R &= ~0x0F;     								// disable analog functionality on Port M	pins (PM0-PM3)	
	return;
}

void PortJ_Init(void){
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R8;					// Activate clock for Port J
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R8) == 0){};			// Allow time for clock to stabilize
  GPIO_PORTJ_DIR_R &= ~0x02;    									// Make PJ1 input 
  GPIO_PORTJ_DEN_R |= 0x02;     									// Enable digital I/O on PJ1
	
	GPIO_PORTJ_PCTL_R &= ~0x000000F0;	 						// Configure PJ1 as GPIO 
	GPIO_PORTJ_AMSEL_R &= ~0x02;								// Disable analog functionality on PJ1		
	GPIO_PORTJ_PUR_R |= 0x02;									// Enable weak pull up resistor on PJ1
}

int delay = 200;
void rotateMotor(int numMeasurements) {
			for(int i=0; i<512/numMeasurements ; i++){												
			GPIO_PORTM_DATA_R = 0b00000011;
			SysTick_Wait10us(delay);											 
			GPIO_PORTM_DATA_R = 0b00000110;													
			SysTick_Wait10us(delay);
			GPIO_PORTM_DATA_R = 0b00001100;													
			SysTick_Wait10us(delay);
			GPIO_PORTM_DATA_R = 0b00001001;													
			SysTick_Wait10us(delay);
	}
}

void undoRotation(void) {
		for(int i=0; i<512 ; i++){										

		GPIO_PORTM_DATA_R = 0b00000011;
		SysTick_Wait10us(delay);											
		GPIO_PORTM_DATA_R = 0b00001001;													
		SysTick_Wait10us(delay);
		GPIO_PORTM_DATA_R = 0b00001100;													
		SysTick_Wait10us(delay);
		GPIO_PORTM_DATA_R = 0b00000110;													
		SysTick_Wait10us(delay);
	}
	GPIO_PORTM_DATA_R = 0;	
}

uint16_t	dev = 0x29;			//address of the ToF sensor as an I2C slave peripheral
int status=0;
uint8_t byteData, sensorState=0, myByteArray[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} , i=0;
uint16_t wordData;
uint16_t Distance;
uint16_t SignalRate;
uint16_t AmbientRate;
uint16_t SpadNum; 
uint8_t RangeStatus;
uint8_t dataReady;

void ToF_Init(void){
		
	// 1 Wait for device booted
	while(sensorState==0){
		status = VL53L1X_BootState(dev, &sensorState);
		SysTick_Wait10ms(10);
  }
	FlashAllLEDs();
	
	status = VL53L1X_ClearInterrupt(dev);  //clear interrupt has to be called to enable next interrupt

  status = VL53L1X_SensorInit(dev);
	
  status = VL53L1X_StartRanging(dev) ;   // This function has to be called to enable the ranging
}


int main(void) {
  
	//Modify these values as required
	int numMeasurements = 64;
	uint8_t numSlices = 3;
	uint8_t seconds = 1;
	//initialize
	PLL_Init();
	PortM_Init();
	PortJ_Init();
	PortG_Init();
	SysTick_Init();
	onboardLEDs_Init();
	I2C_Init();
	UART_Init();

	int input = 0;


	ToF_Init();
	while(1){
		input = UART_InChar();
			if (input == 's')
				break;
	}
	
	//Wait for PJ1 to be pressed
	while (GPIO_PORTJ_DATA_R & 0x02) {}
	// Get the Distance Measurements
	for (int s = 0; s < numSlices; s++) {
	SysTick_Wait10ms(100*seconds); //Small pause in between measurments
	//Communicate with MATLAB
	UART_printf("Ready\r\n");
	SysTick_Wait10ms(1);
	for(int i = 0; i < numMeasurements; i++) {
		uint32_t sumDistance = 0;
    int samplesToTake = 3; 
    int validSamples = 0;
		// Wait until the ToF sensor's data is ready
		for (int i = 0; i < samplesToTake; i++) {
	  while (dataReady == 0){
		  status = VL53L1X_CheckForDataReady(dev, &dataReady);
          VL53L1_WaitMs(dev, 2);
	  }
		dataReady = 0;
		//7 read the data values from ToF sensor
		status = VL53L1X_GetRangeStatus(dev, &RangeStatus);
	  status = VL53L1X_GetDistance(dev,&Distance);					//The Measured Distance value
		//Flash distance measurment LED
		FlashLED3(1);
		//Checkk if error free
		if (status == 0 && RangeStatus == 0) {
        sumDistance += Distance;
        validSamples++;
    }
		status = VL53L1X_GetSignalRate(dev, &SignalRate);
		status = VL53L1X_GetAmbientRate(dev, &AmbientRate);
		status = VL53L1X_GetSpadNb(dev, &SpadNum);

	  status = VL53L1X_ClearInterrupt(dev);  // clear interrupt has to be called to enable next interrupt
		
	}
		//Average valid samples
		if (validSamples > 0) {
          uint16_t averageDistance = sumDistance / validSamples;
          sprintf(printf_buffer, "%u\r\n", averageDistance);
          UART_printf(printf_buffer);        
		} 
		//Send error code and flash error LED
		else {               
			UART_printf("0\r\n");   
			FlashLED2(1);
     }
		//Flash UART LED
		FlashLED1(1);
		rotateMotor(numMeasurements);
		SysTick_Wait10us(delay);
}
  undoRotation();
}
	VL53L1X_StopRanging(dev);
  while(1) {
		GPIO_PORTG_DATA_R |= 0x01;   // PG0 High
    SysTick_Wait10ms(1);         // Wait exactly 340000 cycles
    GPIO_PORTG_DATA_R &= ~0x01;  // PG0 Low
    SysTick_Wait10ms(1);         // Wait exactly 340000 cycles
	}
	
}

