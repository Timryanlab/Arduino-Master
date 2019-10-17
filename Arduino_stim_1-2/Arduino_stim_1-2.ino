// Arduino Stim v 1.2
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
// 1.2 Added switcher support as a switch statement AJN 9-29-19

// USer Variables
int in = 2; // Specify number of inputs used
int out = 3; //Specify number of outputs used
int ins[] = {13, 12}; // {Camera, Ext. Shut}
int outs[] = {5, 4, 3}; // {Stimulate, Laser1, Laser2, ...}

// Initialization of user independent variables.
unsigned int count = 0; // Initialize Frame number (no negative frames)
bool cam0 = LOW; // Initialize 'previous' camera state as low
bool cam = LOW; // Initialize 'current' state as high
int stim = -1;  // Initialize no stimulation unless asked for it (setting to -1 prevents stimulation)
int laser_state = 0; // laser state to induce switching
bool las_stat = 0;

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

  
  int cmd = 0; // Command variable for updating arduino state through USB
  cam = digitalRead(ins[0]); // Read camera state for this frame

  if(cam0 != cam) { // If camera state has changed execute following
    switch(cam){ // Switch between high and low states
      case HIGH: // Indicates Frame is being read
        if(digitalRead(ins[1]) == HIGH || in == 1){ // If either 1 input or the external shutter is open
          switch(laser_state){
          case = 0: // all on or off
            for(int i=1; i < out; i++){ // Loop over laser outputs
              digitalWrite(outs[i],HIGH); // write outputs to high
            }
          case = 1: //switcher between laser1 and laser2
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
        }
        count++; // increment counting variable
        if(count == stim){ // If frame number equals stimulation number
          digitalWrite(outs[0],HIGH); // Send stimulation signal high
          digitalWrite(outs[0],LOW); // send stimulation signal low
          Serial.print("STIM!\n"); // Print 'STIM!' to USB serial
        } 
        Serial.print(count); // print camera frame
        Serial.print("\n"); // newline character
        break; // Don't execute any more of the switch loop
      
      case  LOW:  // Indicates the frame has finished aquistion
        for(int i=1; i < out; i++){ // turn off lasers
          digitalWrite(outs[i],LOW); // write pin low
        } // all other work is taken care of on the rising edge. However if there is a use for the falling edge it would go here
        break;
    }
  } // end state change section
  cam0 = cam; // update previous state to current state
  if(Serial.available() >0){ // if there is a usb input, execute following
    cmd = Serial.read(); // Read the 1 byte input
    Serial.print("Confirmed "); // Respond to USB
    switch(cmd){ // switch statement allows for building of a variety of command interfaces
      case 114: // if the incoming byte is 'r'
        count = 0; // reset the counting variable to 0
        Serial.print("count = 0\n"); // indicate change
        break;
      case 115: // If incoming byte is 's' the following bytes will be a framenumber
        stim = Serial.parseInt(); // parse the bytes into integers
        Serial.print("stimulus is now on frame "); 
        Serial.print(stim);
        Serial.print("\n"); // This confirms the correct stimulus has been saved
        break;
      case 83: // If the incoming bit is 'S' then stimulate now
        digitalWrite(outs[0],HIGH); // Send stimulation output high
        Serial.print("STIM!\n"); // communicate sitmulation
        digitalWrite(outs[0],LOW);  // send stimulation output low
        break;
        case 119: // If the incoming bit is 'S' then stimulate now
        digitalWrite(outs[0],HIGH); // Send stimulation output high
        Serial.print("STIM!\n"); // communicate sitmulation
        digitalWrite(outs[0],LOW);  // send stimulation output low
        break;
      default: // occurs when we encounter and 'unexpected' case
        Serial.print("instructions unclear you gave "); // Indicates no handling of this case
        Serial.print(cmd); // prints out decimal number for future coding
        Serial.print("\n");
    }
  } // end serial loop
} // end main loop

