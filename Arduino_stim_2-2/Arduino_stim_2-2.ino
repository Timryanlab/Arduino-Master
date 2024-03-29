// Arduino Stim v 2.2
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
// 'S' stimulate train immediately
// 'f#' set frequency of train
// 'N#' set number of stimulations / train
// 'p'
//
// Change_log
// 2.2 Added inverter functionality and dual camera capability
// 2.1 Fully functional arduino program, incorporating arming protocol and streamlining communication
// 1.3 Added function generator support AJN 9-29-19
// 1.2 Added switcher support as a switch statement AJN 9-29-19

// USer Variables
int in = 1; // Specify number of inputs used
int out = 4; //Specify number of outputs used
int ins[] = {13, 12}; // {Camera, Ext. Shut}
int outs[] = {7, 2, 3, 4}; // {Stimulate, Laser1, Laser2, ...}
int ext_trig = 5;
int f = 20; // initial frequency of Stimulation in Hz
int N = 20; // Number of stimuli
long pwidth = 1; // pulse width in milliseconds

// Initialization of user independent variables.
unsigned int count = 0; // Initialize Frame number (no negative frames)
bool cam0 = LOW; // Initialize 'previous' camera state as low
bool cam = LOW; // Initialize 'current' state as high
bool cam20 = LOW; // Initialize 'previous' camera state as low
bool cam2 = LOW; // Initialize 'current' state as high
bool invert = LOW;
bool simul = LOW;
bool ext_go = LOW;
int arm = 0;
int stim = 10;  // Initialize no stimulation unless asked for it (setting to -1 prevents stimulation)
int laser_state = 0; // laser state to induce switching
long lastmil = 0; // Storage variable for previous time
long lastcam = 0;
long cam_p = 100; // initialization of camera period

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
  pinMode(ext_trig,OUTPUT);
  digitalWrite(ext_trig,LOW);
  pinMode(ins[1],INPUT);
  // This section is for USB communication early on and will be streamlined after debugging 8-29-19
  Serial.begin(9600);
  Serial.setTimeout(100); // we won't be communicating that much, 100 ms should be fine
  Serial.print("Stimulus set to frame ");
  Serial.print(stim);
  Serial.print("\n");

}

void loop() {

  unsigned long nowt = millis();
  if(nowt-lastcam >= cam_p && ext_go){
    take_frame();
    lastcam = nowt;
    } // external camera synchronization pulse
  
  int cmd = 0; // Command variable for updating arduino state through USB
  
  cam = digitalRead(ins[0]); // Read camera state for this frame
  cam2 = digitalRead(ins[1]);
  
  if(invert == HIGH){
    cam = !cam;
    cam2 = !cam2;
    } // invert camera signal if invert is high
  
  if(cam0 != cam) { // If camera state has changed execute following
    switch(cam){ // Switch between high and low states
      case HIGH: // Indicates Frame is being read
       // if(digitalRead(ins[1]) == HIGH || in == 1){ // If either 1 input or the external shutter is open
          switch(laser_state){
          case 0: // all on or off
            for(int i=1; i < out; i++){ // Loop over laser outputs
              digitalWrite(outs[i],HIGH); // write outputs to high
            }
            break;
          case 1: //switcher between laser1 and laser2
            switch(count%2){
              case 0:
                digitalWrite(outs[2],HIGH);
                break;
              case 1:
                digitalWrite(outs[1],HIGH);
                break;
            }//end switch las_stat for switcher
            break;
          case 2: // if laser state is set to 2, we have each camera controlling it's own laser
            digitalWrite(outs[3],HIGH);
            break;
          } // end switch laser state
        
          count++; // increment counting variable
          if(count == stim){
            if(arm == 1){flag = 1;}
          }
          Serial.print(count); // print camera frame
          Serial.print("\n"); // newline character
       // }
        break; // Don't execute any more of the switch loop
    
      case  LOW:  // Indicates the frame has finished aquistion
        switch(laser_state){
          case 2: // turn off only camera's lasers
            digitalWrite(outs[3],LOW); // write pin low
            digitalWrite(outs[2],LOW);
            break;
          default: // default should be all lasers off at the end of the frame
            for(int i=1; i < out; i++){ // turn off lasers
              digitalWrite(outs[i],LOW); // write pin low
            } // all other work is taken care of on the rising edge. However if there is a use for the falling edge it would go here
            break;
        }
    }
  } // end state change section
  cam0 = cam; // update previous state to current state
  if(simul == HIGH){ // if simul is high we want 2 cameras each controlling their own laser
    if(cam20 != cam2) { // If camera state has changed execute following
      switch(cam2){ // Switch between high and low states
        case HIGH: // Indicates Frame is being read
          digitalWrite(outs[1],HIGH);   
          break;
        case LOW:
          digitalWrite(outs[1],LOW);
          
          break;
      }
    }
    
  }
  cam20 = cam2;
  
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
      
      case 78: // if incoming bit is 'N' change number of stimuli                               // variables and commands used
        N =Serial.parseInt(); // Parse incoming integer into N                                  // 78: N: changes number of stimuli
        Serial.print("Number of stimuli is now "); // Confirm Input                             // 83: S: causes immediate execution of stimulation train
        Serial.print(N); // Repeat input for confirmation                                       // 97: a: changes which frame stimulation will occur on
        Serial.print("\n"); // newline                                                          // 99: c#: changes the time between camera frames for external trigger
        break;                                                                                  // 101: e: toggle external triggering
                                                                                                // 102: f#:  sets frequency of stimulation train 
      case 83: // If the incoming bit is 'S' then stimulate now                                 // 105: i: toggle inverter
        train(); // Run a train of stimuli at p intervals                                       // 107: k: toggle simultaneous camera acquisition
        Serial.print("STIM!\n"); // communicate sitmulation                                     // 114: r: reset frame number
        break;                                                                                  // 115: s: force train of stimulation 
                                                                                                // 119: w: toggle switcher
      case 97: // If incoming byte is 'a' then arm stimulation                                  //
        arm = (arm +1) % 2;                                                                     //
        Serial.print("Arm is now "); // Confirm input                                           //
        Serial.print(arm); // Repeat input for confirmation                                     //
        Serial.print("\n"); // newline                                                          //
        break;                                                                                  //
      case 99: // if incoming byte is 'c' change camera cycle time (cam_p)                      //
        cam_p = Serial.parseInt();                                                              //
        Serial.print("Delay between camera frames is now ");                                    //
        Serial.print(cam_p);                                                                    //
        Serial.print("ms\n");                                                                   //
        break;                                                                                  //
                                                                                                //
      case 101: // if incoming bit is 'e' toggle external triggering                            //
        ext_go = !ext_go;                                                                       //  
        lastcam = nowt;                                                                         //
        for(int i =1; i<outs; i++){digitalWrite(outs[i],LOW);}                                  //
        Serial.print("Ext go is set to "); // Confirm Input                                     //
        Serial.print(ext_go); // Repeat input for confirmation                                  //
        Serial.print("\n"); // newline                                                          //
        break;
      case 102: // if incoming bit is 'f' reset frequency
        f = Serial.parseInt(); // parse the bytes into integers
        p = 1000/f; // recalculate period
        Serial.print("stimulus is now at "); // Confirm Input
        Serial.print(f); // Repeat input for confirmation
        Serial.print("Hz\n"); // newline
        break;
        
      case 105: // if incoming bit is 'i' toggle inverter
        invert = !invert;
        Serial.print("Invert is set to "); // Confirm Input
        Serial.print(invert); // Repeat input for confirmation
        Serial.print("\n"); // newline
        break;  

        case 107: // if k is in toggle simultaneous 
        simul = !simul;
        if(laser_state == 2){
          laser_state = 0;
          for(int i =1; i<outs; i++){digitalWrite(outs[i],LOW);}
        }
        else{laser_state = 2;}
        Serial.print("Simultaneous is set to "); // Confirm Input
        Serial.print(simul); // Repeat input for confirmation
        Serial.print("\n"); // newline
        break;
        
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
      
      case 119: // If the incoming bit is 'w' then turn on/off switcher
        if(laser_state == 1){laser_state = 0;}
        else if(laser_state == 0 ){laser_state = 1;}
        Serial.print("Switcher Toggled\n"); // Issue confirmation
        break;
      

      
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

void take_frame(){ // stimulate function
  digitalWrite(ext_trig,HIGH);  // write pin high
  delay(pwidth);              // wait pulse width time
  digitalWrite(ext_trig,LOW);  // write pin low
}

void train(){  // run a stimulation train at known intervals
  for(n = 0; n < N; n++){ // run for N stims
    stimulate(); // stimulate
    delay(p); // delay by period and repeat
  }
}
