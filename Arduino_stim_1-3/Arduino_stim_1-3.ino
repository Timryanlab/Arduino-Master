// Arduino Stim v 1.3
//
// This is an arduino sketch attempting to handle the electrical communication between
// an EMCCD, Isolator Battery, Coherent Laser box, in a controlled and programable fashion. 
// This version is a working version that will drive up to a number of digital outputs.
// It operates by reading the rising edge of camera frame. When the camera frame is high,
// the digital signal for lasers goes high, then turn off with the camera fire line. The 
// Stimulator turns on for a specified frame.
// AN 8-29-2019 Tim Ryan Lab
//
// User commands while running
// 'r' reset counting
// 's#' set frame # to new stimulation value
// 'S' stimulate immediately
//
// Change_log
// 1.3 Added function generator support AJN 9-29-19
// 1.2 Added switcher support as a switch statement AJN 9-29-19

// USer Variables
int in = 2; // Specify number of inputs used
int out = 3; //Specify number of outputs used
int ins[] = {13, 12}; // {Camera, Ext. Shut}
int outs[] = {5, 4, 3}; // {Stimulate, Laser1, Laser2, ...}
int f = 1; // initial frequency of Stimulation in Hz
int N = 10; // Number of stimuli
unsigned long pwidth = 1; // pulse width in milliseconds

// Initialization of user independent variables.
unsigned int count = 0; // Initialize Frame number (no negative frames)
bool cam0 = LOW; // Initialize 'previous' camera state as low
bool cam = LOW; // Initialize 'current' state as high
int stim = 10;  // Initialize no stimulation unless asked for it (setting to -1 prevents stimulation)
int laser_state = 0; // laser state to induce switching
bool las_stat = 0; // laser status variable for switching
long lastmil = 0; // Storage variable for previous time

unsigned long p = 1000/f; // Period between stimuli in ms
int flag = 0; // flag for timing

int n = 0; // stimulus counting variable



void setup() {
  for (int i =0; i< in; i++){ // loop over inputs
    pinMode(ins[i],INPUT);  // activate inputs
  }
  for (int i =0; i< out; i++){ // activate outputs
    pinMode(outs[i],OUTPUT);
    digitalWrite(outs[i], LOW); // initialize outputs to low
  }

  // This section is for USB communication early on and will be streamlined after debugging 8-29-19
  Serial.begin(9600);
  Serial.setTimeout(100); // we won't be communicating that much, 100 ms should be fine
  Serial.print("Stimulus set to frame ");
  Serial.print(stim);
  Serial.print("\n");
  if(laser_state < 1){
    las_stat = 0;
  }
}

void loop() {

  unsigned long nowt = millis();
  int cmd = 0; // Command variable for updating arduino state through USB
  cam = digitalRead(ins[0]); // Read camera state for this frame
  if(cam0 != cam) { // If camera state has changed execute following
    switch(cam){ // Switch between high and low states
      case HIGH: // Indicates Frame is being read
        if(digitalRead(ins[1]) == HIGH || in == 1){ // If either 1 input or the external shutter is open
          switch(laser_state){
          case 0: // all on or off
            for(int i=1; i < out; i++){ // Loop over laser outputs
              digitalWrite(outs[i],HIGH); // write outputs to high
            }
          case 1: //switcher between laser1 and laser2
            switch(las_stat){
              case 0:
                digitalWrite(outs[las_stat+1],HIGH);
                las_stat = 1;
                break;
              case 1:
                digitalWrite(outs[las_stat+1],HIGH);
                las_stat = 0;
                break;
            }//end switch las_stat for switcher
          } // end switch laser state
        
        count++; // increment counting variable
        if(count == stim){flag = 1;}
        Serial.print(count); // print camera frame
        Serial.print("\n"); // newline character
        }
        break; // Don't execute any more of the switch loop
    
      case  LOW:  // Indicates the frame has finished aquistion
        for(int i=1; i < out; i++){ // turn off lasers
          digitalWrite(outs[i],LOW); // write pin low
        } // all other work is taken care of on the rising edge. However if there is a use for the falling edge it would go here
        break;
    }
  } // end state change section
  cam0 = cam; // update previous state to current state

  // Stimulation Section
  if(flag == 1){ // if stimulation flag is high, look for stimulation behavior
    if(n == 0){ // if no stimuli have been given
      stimulate(); // stimulate
      //Serial.print(n);
      lastmil = nowt; // update time
      n = n+1;  // increment stim count
    }
    
    else{ // if one stimulus has already been given, wait p amount of milliseconds
      if(nowt - lastmil >= p && n<N){ // if delay has been long enough AND maximum number of stimuli hasn't been reached
        stimulate(); // stimulate
        lastmil = nowt; // update time
        n=n+1; // increment stim count
      }
    }
    
    if(n == N){n=0; flag = 0;} // reset stim count and flag
  }
  
  // Command Update Section
  if(Serial.available() >0){ // if there is a usb input, execute following
    cmd = Serial.read(); // Read the 1 byte input
    Serial.print("Confirmed "); // Respond to USB
    switch(cmd){ // switch statement allows for building of a variety of command interfaces
      case 114: // if the incoming byte is 'r'
        count = 0; // reset the counting variable to 0
        Serial.print("count = 0\n"); // Confirm change
        break;
      case 115: // If incoming byte is 's' the following bytes will be a framenumber
        stim = Serial.parseInt(); // parse the bytes into integers
        Serial.print("stimulus is now on frame "); // Confirm input
        Serial.print(stim); // Repeat input for confirmation
        Serial.print("\n"); // newline
        break;
      case 83: // If the incoming bit is 'S' then stimulate now
        train(); // Run a train of stimuli at p intervals
        Serial.print("STIM!\n"); // communicate sitmulation
        break;
      case 119: // If the incoming bit is 'w' then turn on/off switcher
        if(laser_state == 1){laser_state = 0;}
        else if(laser_state == 0 ){laser_state = 1;}
        Serial.print("Switcher Toggled\n"); // Issue confirmation
        break;
      case 102: // if incoming bit is 'f' reset frequency
        f = Serial.parseInt(); // parse the bytes into integers
        p = 1000/f; // recalculate period
        Serial.print("stimulus is now at "); // Confirm Input
        Serial.print(f); // Repeat input for confirmation
        Serial.print("Hz\n"); // newline
        break;
      case 78: // if incoming bit is 'N' change number of stimuli
        N =Serial.parseInt(); // Parse incoming integer into N
        Serial.print("Number of stimuli is not "); // Confirm Input
        Serial.print(N); // Repeat input for confirmation
        Serial.print("\n"); // newline
        break;
      case 80: // if incoming bit is 'P' change pulse width
        pwidth = Serial.parseInt(); // parse integer into pulse width in ms
        Serial.print("Pulse width is now "); // Confirm Input
        Serial.print(pwidth); // Repeat input for confirmation
        Serial.print("ms\n"); // newline
      default: // occurs when we encounter and 'unexpected' case
        Serial.print("instructions unclear you gave "); // Indicates no handling of this case
        Serial.print(cmd); // prints out decimal number for future coding
        Serial.print("\n");
    }
  } // end serial loop
} // end main loop

void stimulate(){ // stimulate function
  digitalWrite(outs[0],HIGH);  // write pin high
  delay(pwidth);              // wait pulse width time
  digitalWrite(outs[0],LOW);  // write pin low
}

void train(){  // run a stimulation train at known intervals
  for(n = 0; n < N; n++){ // run for N stims
    stimulate(); // stimulate
    delay(p); // delay by period and repeat
  }
}

