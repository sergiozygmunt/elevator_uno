/*
This is a project that started with the H-Bridge sketch from Arduino Cookbook page 283 and
the PING sensor tutorial at http://www.arduino.cc/en/Tutorial/Ping.  Also has some elements
of the basic "getting input from a button" Arduino tutorial. 

There is a PING sensor that constantly locates an "elevator car" on rails.  When an "Up" button
is pressed, the motor runs, acting as a winch, to lift the car.  When the "Down" button is pressed,
the motor runs the other direction.

The program below defines three "floors", with the ground floor position at 3 cm, and each "floor"
being 6 cm high.

Pressing an Up button will add 6cm to the destination distance amount, and the motor will run until
it reaches that position, and stop.  Pressing a down button, when the car is not at the "ground" level,
will run the motor the other way to allow the car to move down.

The loop() function will get the distance from the sensor to the elevator car
and control the motor to move the car in the right direction to get to the 
distance stored in the variable called destinationPosition.

*/

//up and down tact buttons
int btnDown = 2; 
int btnUp = 3;

//H-Bridge pins
const int hBridgeInputPin2 = 4; //h-bridge input pin 2
const int hBridgeInputPin1 = 5; //h-bridge input pin 1
const int hBridgeEnablePin = 6; //h-bridge enable pin

//The pin number of the PING)) sensor's output.
const int pingPin = 7;

//Pin Definitions for 7-segment LED, which displays the floor number or indicates
//when the car is moving. 
//The 74HC595 uses a protocol called SPI (for more details http://www.arduino.cc/en/Tutorial/ShiftOut)
int clock = 8; 
int latch = 9; 
int data = 10; 

const int groundFloorPositionCm = 4;  //bottom of first floor is 4 cm from sensor
const int topFloorPositionCm = 18; //bottom of the third floor is at 18 cm from sensor
const int floorHeightCm = 7;  //a "floor" is 6 cm high.  This allows 3 floors betw 4 and 18 cms.                

//the given numbers are what to send to the shift register in order to make the 7-segment
//LED display the number that is in comments  (e.g. send 64 to the shift register to display "0")
int ledNumbers [] = { 64,  //0,
                      121, //1
                      36,  //2
                      48,  //3
                      25,  //4
                      18,  //5
                      2,   //6
                      120, //7
                      0,   //8
                      24,};  //9 

//this stores the current position of the car - this value is updated each time the main loop code executes
int currentPosition = -1; 

//floor do display in the7-segment LED.  This is stored becuase we don't want to continually calculate it. 
//as the elevator moves between floors, it will show the last floor reached until it stops, at which point it
//will update the number with the current floor. 
int lastKnownFloorNumber = 0;

//this is the current destination, in cms from the sensor
//the motor will run in the correct direction to reach this position, then it will stop.
int destinationPosition = -1;  
int lastDestinationReached = -1; 

//pin for the piezo buzzer
int speakerPin = 12;

void setup()
{
  //set up serial communication to receive debugging information
  Serial.begin(9600);
  
  //set up pins for the h-bridge
  pinMode(hBridgeInputPin1, OUTPUT); 
  pinMode(hBridgeInputPin2, OUTPUT); 
  
  digitalWrite(hBridgeInputPin1, LOW); 
  digitalWrite(hBridgeInputPin2, HIGH);
  
  //up and down button pins are used for input
  pinMode(btnUp, INPUT);
  pinMode(btnDown, INPUT);

  //set up pins for shift register - these are outputs 
  pinMode(data, OUTPUT);
  pinMode(clock, OUTPUT);  
  pinMode(latch, OUTPUT);
  
  pinMode(speakerPin, OUTPUT);
  
  //set the initial destination to be the first floor by default.  When the loop() function starts running,
  //it will move the car to the initial position if it's not already there.
  InitializeElevatorPosition(); 
}

void loop()
{
  currentPosition = GetCurrentCarPositionCms(); 
  
  ReadButtons(); 
 
  ControlMotor();  
}

void ReadButtons()
{
  //the car is moving - we don't read buttons while it's moving
  if (currentPosition != destinationPosition)
  {
    return; 
  }

  DisplayCurrentFloorNumber(); 
   
  //was the Up button pressed? If the car is stopped, adjust the destination distance, and run the motor to go up.
  if (digitalRead(btnUp) == LOW) 
   {
     if (currentPosition < topFloorPositionCm)
       destinationPosition += floorHeightCm; 
     
     return; 
  }

   //was the Down button pressed? If the car is stopped, reduce the destination distance and run the motor
   if (digitalRead(btnDown) == LOW) 
   {
      if (currentPosition > groundFloorPositionCm)
        destinationPosition -= floorHeightCm; 

      return; 
    }
}

//when the program starts, move it to the "ground" floor
void InitializeElevatorPosition()
{
  destinationPosition = groundFloorPositionCm;
}

//using the PING))), get the number of centimeters from the sensor
//to the bottom of the elevator car.
int GetCurrentCarPositionCms()
{
  // establish variables for duration of the ping,
  // and the distance result in inches and centimeters:
  long duration, inches, cm;

  // The PING))) is triggered by a HIGH pulse of 2 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  pinMode(pingPin, OUTPUT);
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(pingPin, LOW);

  // The same pin is used to read the signal from the PING))): a HIGH
  // pulse whose duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  pinMode(pingPin, INPUT);
  duration = pulseIn(pingPin, HIGH);

  return MicrosecondsToCentimeters(duration);
}

// Runs motor, if necessary, in the right direction to reach the 
// current destination.  Stops motor (or keeps motor stopped) if at the destination position
void ControlMotor()
{
  ////we're in the position we want - stop the motor, or leave it stopped.
  if (currentPosition == destinationPosition)
  {    
    // stop the car
    analogWrite(hBridgeEnablePin, 0);
    if (destinationPosition != lastDestinationReached)
    {
      delay(1000); 
      PlayTone(1915, 2000); 
      lastDestinationReached = destinationPosition;
      }
      
    return; 
  }

  if (currentPosition > destinationPosition) 
  {
    //destination is below the current position - run the motor clockwise to lower the car
    digitalWrite(hBridgeInputPin1, HIGH); 
    digitalWrite(hBridgeInputPin2, LOW); 
  }
  else 
  {
    //run motor clockwise - lower the car
    digitalWrite(hBridgeInputPin1, LOW); 
    digitalWrite(hBridgeInputPin2, HIGH); 
  }

  //make sure the motor is running
  //use the map() function to control the speed of the motor (between 0 and 255)
  analogWrite(hBridgeEnablePin, 255); 
}


void PlayTone(int tone, int duration) 
{
  for (long i = 0; i < duration * 1000L; i += tone * 2) 
  {
    digitalWrite(speakerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(speakerPin, LOW);
    delayMicroseconds(tone);
  }
}

int DisplayCurrentFloorNumber()
{
  //Turn the current car position into a sort-of floor number.  The number that will be computed in the next line of code
  //may be 2.0, 2.4, 1.3, etc.  Those aren't floor numbers that would be displayed in an elevator, but for our purposes, 
  //they are used to measure the position of the car with respect to the floors.  
  //This code uses a float to allow a decimal place to be used with the floor number to indicate partial floors.
  //Using an integer would not allow the partial floor designations (i.e. decimal point)
  float floorNumber = (float)((float)(currentPosition - groundFloorPositionCm) / (float)floorHeightCm) + (float)1.0; 

  if (lastKnownFloorNumber != (int)floorNumber)
    lastKnownFloorNumber =(int)floorNumber; 
  
  OutputFloorNumber(lastKnownFloorNumber);   
}

/*
* OutputFloorNumber() - sends the LED states set in ledStates to the 74HC595
* sequence
*/
void OutputFloorNumber(int value)
{
  //Pulls the chips latch low
  digitalWrite(latch, LOW);     

  //Shifts out the 8 bits to the shift register.  This is a built-in Arduino function.
  shiftOut(data, clock, MSBFIRST, ledNumbers[value]); 
  
  //Pulls the latch high displaying the data
  digitalWrite(latch, HIGH);   
}

long MicrosecondsToCentimeters(long microseconds)
{
  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the
  // object we take half of the distance travelled.
  
  //http://www.arduino.cc/en/Tutorial/Ping
  
  return microseconds / 29 / 2;
}
