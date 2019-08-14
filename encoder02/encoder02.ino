
#include <PID_v1.h>
//input:velocity from computer sent through ros
//output:pwm to motor controller
#include <ros.h>
#include <Arduino.h>
#include <std_msgs/Float64.h>
#include <std_msgs/Int32.h>

ros::NodeHandle  nh;

//*** Below are the pin configurations for both encoders. A is Green wire, B is White wire, Z is Yellow wire, Red and Black are 5V VCC and GND.
#define encoder0PinA 2
#define encoder0PinB 3
#define encoder0PinZ 18
#define encoder1PinA 19
#define encoder1PinB 20
#define encoder1PinZ 21
//*** Pin configurations for the motor controllers. Accel means rear motor controller, Direc means front motor controller
#define accelPWM 9
#define accelDIR 8
#define direcPWM 6
#define direcDIR 7

//*** Parameters for Acceleration PID
#define kpA 3
#define kiA 0
#define kdA 0

//*** Parameters for Direction PID
#define kpD 3
#define kiD 0.3
#define kdD 0

//output conencts to potentiometer of the motor controller's arduino?


double vCurrent, vTarget; //current and target speed variables
double target_angle; // targe
int accelpwm; //variable for pwm value for acceleration
int direcpwm; //variable for pwn value for direction

//unsigned volatile short pulse0;
//unsigned volatile short pulse1;

PID accelPID (&vCurrent, &accelpwm, &vTarget, kpA, kiA, kdA, DIRECT);
PID direcPID (&front_angle, &direcpwm, &target_angle, kpD, kiD, kdD, DIRECT);

volatile int encoder0Pos = 0;
//int encoder0; //encoder0Pos before for testing CCW OR CW
//int encoder0_b; //mark the encoder0Pos when pulses change

volatile int encoder1Pos = 0;
//int encoder1; //encoder1Pos before for testing CCW OR CW
//int encoder1_b; //mark the encoder0Pos when pulse changes (this means that a revolution for the rear wheel has passed)


//std_msgs::String str_msg;
int left_encoder;
int right_encoder;
double left_speed;
double right_speed;
double front_angle;


//fetch current left wheel speed from ROS
void currentSpeed_left(const std_msgs::Float64 &val) {
  left_speed = val.data;
} 

//fetch current right wheel speed from ROS
void currentSpeed_right(const std_msgs::Float64 &val) {
  right_speed = val.data;
}

//fetch current RAW data from absolute encoder from ROS and convert it to radians
void currentRadian_front(const std_msgs::Int32 &val) {
  front_angle = ((double)val.data)*2*pi/1024;
}

void targetSpeed(const std_msgs::Float64 &val) {
  vTarget = val.data;
}

void targetRadian(const std_msgs::Float64 &val) {
  target_angle = val.data;
}


//"" is the topic name, pc...--name of subscriber
//ros::Subscriber<std_msgs::Int32> pc_vTarget("motor_vTarget", &updateVTarget);
ros::Publisher lwheel_tick("lwheel", left_encoder);
ros::Publisher rwheel_tick("rwheel", right_encoder);
ros::Subscriber<std_msgs::Float64> lwheel_speed("lwheel_speed", &currentSpeed_left);  //current left wheel speed calculated by computer
ros::Subscriber<std_msgs::Float64> rwheel_speed("rwheel_speed", &currentSpeed_right);  //current right wheel speed calculated by computer
ros::Subscriber<std_msgs::Int32> fwheel_tick("fwheel_tick", &currentRadian_front); //current RAW data from absolute encoder in front
ros::Subscriber<std_msgs::Float64> control_speed("control_speed", &targetSpeed); //target speed from computer
ros::Subscriber<std_msgs::Float64> control_steering("control_steering", &targetRadian); //target radians from computer

void setup() {
  nh.initNode();  
  nh.subscribe(lwheel_speed);
  nh.subscribe(rhweel_speed);
  nh.subscribe(fwheel_tick);
  nh.advertise(lwheel_tick);
  nh.advertise(rwheel_tick);
  nh.subscribe(control_speed);
  nh.subscribe(control_steering);
  
  
  pinMode(encoder0PinA, INPUT);
  digitalWrite(encoder0PinA, HIGH);       // turn on pull-up resistor
  pinMode(encoder0PinB, INPUT);
  digitalWrite(encoder0PinB, HIGH);       // turn on pull-up resistor
  pinMode(encoder0PinZ, INPUT);
  digitalWrite(encoder0PinZ, HIGH);       // turn on pull-up resistor
  pinMode(encoder1PinA, INPUT);
  digitalWrite(encoder1PinA, HIGH);       // turn on pull-up resistor
  pinMode(encoder1PinB, INPUT);
  digitalWrite(encoder1PinB, HIGH);       // turn on pull-up resistor
  pinMode(encoder1PinZ, INPUT);
  digitalWrite(encoder1PinZ, HIGH);       // turn on pull-up resistor
  pinMode(MOTORL, OUTPUT);
  
  attachInterrupt(digitalPinToInterrupt(encoder0PinA), doEncoderA(0), CHANGE);  // Output channel A from encoder 0 -> interrupt pin 2
  attachInterrupt(digitalPinToInterrupt(encoder0PinB), doEncoderB(0), CHANGE);  // Output channel B from encoder 0 -> interrupt pin 3
  attachInterrupt(digitalPinToInterrupt(encoder0PinZ), doEncoderC(0), RISING); // Output channel C from encoder 0 -> interrupt pin 18       
  attachInterrupt(digitalPinToInterrupt(encoder1PinA), doEncoderA(1), CHANGE);  // Output channel A from encoder 1 -> interrupt pin 2
  attachInterrupt(digitalPinToInterrupt(encoder1PinB), doEncoderB(1), CHANGE);  // Output channel B from encoder 1 -> interrupt pin 3
  attachInterrupt(digitalPinToInterrupt(encoder1PinZ), doEncoderC(1), RISING); // Output channel C from encoder 1 -> interrupt pin 18          
  
  //accept setPoint value from commands of PC
  //will it get update again if put in the setup() function?
  myPID.SetMode(AUTOMATIC);

  
}

void loop() {
  left_encoder = encoder0Pos;
  right_encoder = encoder1Pos;
  lwheel_tick.publish(&left_encoder);
  rwheel_tick.publish(&right_encoder);
  vCurrent = (left_speed + right_speed) / 2;
  updateVelocity();
}

void doEncoderA(bool encoderNum) {
  if (encoderNum == 0)
  { 
    encoder0 = encoder0Pos;
  /*  if (digitalRead(encoder0PinA) == HIGH){  //Commented out because we no longer need to reset the pulse, computer takes care of speed calc
      pulse0++;
    } */
    if (digitalRead(encoder0PinA) == digitalRead(encoder0PinB)){
      encoder0Pos--;
    }
    else { 
      encoder0Pos++;
    }
/*    if (abs(encoder0) > abs(encoder0Pos)){  //Commented out because we no longer need to reset the pulse, computer takes care of speed calc
      pulse0 = 0;
    }*/
  }
  else
  {
    encoder1 = encoder1Pos;
    /*if (digitalRead(encoder1PinA) == HIGH){  //Commented out because we no longer need to reset the pulse, computer takes care of speed calc
      pulse1++;
    } */
    if (digitalRead(encoder1PinA) == digitalRead(encoder1PinB)){
      encoder1Pos--;
    }
    else { 
      encoder1Pos++;
    }
   /* if (abs(encoder1) > abs(encoder1Pos)){  //Commented out because we no longer need to reset the pulse, computer takes care of speed calc
      pulse1 = 0;
    } */
  }
}

void doEncoderB(bool encoderNum) {
  if (encoderNum == 0){
   // encoder0 = encoder0Pos;
    if (digitalRead(encoder0PinA) == digitalRead(encoder0PinB)){
      encoder0Pos++;
    }
    else {
      encoder0Pos--;
    }
   /* if (abs(encoder0) > abs(encoder0Pos)){  //Commented out because we no longer need to reset the pulse, computer takes care of speed calc
      pulse0 = 0;
    }  */
  }
  else {
   // encoder1 = encoder1Pos;
    if (digitalRead(encoder1PinA) == digitalRead(encoder1PinB)){
      encoder1Pos++;
    }
    else {
      encoder1Pos--;
    }
   /* if (abs(encoder1) > abs(encoder1Pos)){  //Commented out because we no longer need to reset the pulse, computer takes care of speed calc
      pulse1 = 0;
    }  */
  }

}

void doEncoderC(bool encoderNum) {
  if (encoderNum == 0){
    encoder0Pos = 0;
  }
  else{
    encoder1Pos = 0;
  }
}



void updateVelocity(){
  
    accelPID.Compute();
    direcPID.Compute();
    updateAccel(accelpwm, direcpwm);
   /* Seril.println(pwm, DEC);
    analogWrite(PWM, pwm);
    String str_pwm = String(pwm);
    int str_pwm_length = str_pwm.length() + 1;
    char pwm_str_array[str_pwm_length];
    str_pwm.toCharArray(pwm_str_array, str_pwm_length);
    str_msg.data = pwm_str_array;
    pub_vTarget.publish(&str_msg); */
}

//Remember to check whether the directions are correct
void updateAccel(int accelpwm, int direcpwm) {
  if(accelpwm <= 0) {
    digitalWrite(accelDIR, HIGH);
  }
  else {
    digitalWrite(accelDIR, LOW);
  }
  
  if(direcpwm <= 0) {
    digitalWrite(direcDIR, HIGH);
  }
  else {
    digitalWrite(direcDIR, LOW);
  }
  
  analogWrite(accelPWM, abs(accelpwm));
  
  analogWrite(direcPWM, abs(direcpwm));
  
}
