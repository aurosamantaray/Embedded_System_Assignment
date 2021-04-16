#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>

#define BLOCK_SIZE 4096
/* FE000000 is the base address for Rpi 4*/
#define GPIO_PHERIPHERAL_BASE_ADDRESS        0xFE200000

#define GPIO_2 2
#define GPIO_5 5


/* Global variable to hold memory map address of size BLOCK_SIZE*/
volatile unsigned *gpio;
int time_elapsed=1;

/* Convert PWM frequency to time in microseconds */
#define PWM_FREQUENCY(x) ((float)1/x) * 1000000
#define DUTY_CYCLE

/* Set GPIO pin as output */
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))

/* Set and clear logic for GPIO
   gpio + 7 maps to GPFSET0 register
   GPIO + 10 MAPS TO GPFCLR0 register */
#define GPIO_SET *(gpio+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio+10) // clears bits which are 1 ignores bits which are 0


/* Function declarations */
void gpio_config();
void set_led(int);
void timer_handler(int);

/* This is where is all begins */
int main()
{
  /* Configure the desired registers to output */
  gpio_config();
  for(;;){
  set_led(0);
  set_led(50);
  set_led(100);
  }
  return 0;

}
/* 
  @brief : PWM logic to control the brightness of 2 LEDs with complementary duty cycle 
*/
void set_led(int duty_cycle){
  /* PWM is configured for 100Hz or 10000us*/
  int pwm_time_period = PWM_FREQUENCY(100);
  int pwm_high_period = pwm_time_period * ((float)duty_cycle/100);
  int pwm_low_period = pwm_time_period - pwm_high_period;
  printf("Debug : %d---------%d\n",pwm_high_period,pwm_low_period);
  /* Call back function for timer_handler */
  signal(SIGALRM, &timer_handler);
  /* Set timeout to 1 second, this works as a software interrupt to update the value of time_elapsed variable so we can breakout of while loop*/
  alarm(1);
  while(time_elapsed){
      if(pwm_high_period){
		GPIO_SET = (1 << GPIO_2 );
		GPIO_CLR = (1 << GPIO_5);
		usleep(pwm_high_period);
      			}
      if(pwm_low_period){
    		GPIO_CLR = (1 << GPIO_2);
    		GPIO_SET = (1 << GPIO_5);
    		usleep(pwm_low_period);
      			}
     }
     /* Reset the variable to 1 for the next function*/
    time_elapsed = 1;
    /* Clear the GPIO pins on exit*/
    GPIO_CLR = (1 << GPIO_2) | (1 << GPIO_5);
}
/*
  @brief : Configure the gpio pin 2 and 5 to output and get the memory map of GPIO base address 
*/
void gpio_config()
{
    int fd;

   if ((fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
      perror("File open error \n");
      exit(-1);
   }
   
   gpio = mmap(NULL, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, GPIO_PHERIPHERAL_BASE_ADDRESS);
   
   close(fd);
   
   if (gpio == MAP_FAILED) {
      perror("mmap error, mapping failed\n");
      exit(-1);
   }
   /* Set gpio pin 2 and 5 to output*/
    OUT_GPIO(GPIO_2);
    OUT_GPIO(GPIO_5);
}

/* Timer to change the value of time_elapsed */
void timer_handler(int time_period){
  time_elapsed = 0;
}
