// Arduino Stim v 1.1
//
// This is an arduino sketch attempting to handle the electrical communication between
// an EMCCD, Isolator Battery, Coherent Laser box, in a controlled and programable fashion. 
// This version is a working version that will drive up to a number of digital outputs.
// It operates by reading the rising edge of camera frame. When the camera frame is high,
// the digital signal for lasers goes high, then turn off with the camera fire line. The 
// Stimulator turns on for a specified frame.
//
// User commands
// 'r' reset counting
// 's#' set frame # to new stimulation value
// 'S' stimulate immediately
// USer Variables
int in = 2;
int out = 3;
int ins[] = {13, 12}; // {Camera, Ext. Shut}
int outs[] = {5, 4, 3, 2, 6}; // {Stimulate, Laser1, Laser2, ...}

// Initialization
unsigned int count = 0;
bool cam0 = LOW;
bool cam = LOW;
int stim = 10;
void setup() {
  // put your setup code here, to run once:
  for (int i =0; i< in; i++){
    pinMode(ins[i],INPUT);  // activate inputs
  }
  for (int i =0; i< out; i++){ // activate outputs
    pinMode(outs[i],OUTPUT);
    digitalWrite(outs[i], LOW);
  }
  Serial.begin(9600);
  Serial.setTimeout(100); // we won't be communicating that much, 100 ms should be fine
  Serial.print("Stimulus set to frame ");
  Serial.print(stim);
  Serial.print("\n");
}

void loop() {
  // put your main code here, to run repeatedly:
  int cmd = 0;
  cam = digitalRead(ins[0]); // Read camera state
  unsigned long cmil = millis();
  unsigned long avel;
  if(cam0 != cam) { // If camera state has changed execute following
    switch(cam){
      case HIGH: //rising edge
        if(digitalRead(ins[1]) == HIGH || in == 1){
        for(int i=1; i < out; i++){ // turn on lasers
          digitalWrite(outs[i],HIGH); // write pin high
        }
        }
        count++; // increment counting variable
        if(count == stim){digitalWrite(outs[0],HIGH);digitalWrite(outs[0],LOW); Serial.print("STIM!\n");}
        Serial.print(count); // print camera frame
        Serial.print("\n");
        break;
      case  LOW:  // falling edge
        for(int i=1; i < out; i++){ // turn on lasers
          digitalWrite(outs[i],LOW); // write pin high
        }
        break;
    }
  }
  cam0 = cam;
  if(Serial.available() >0){ // if there is a usb input, execute following
    cmd = Serial.read();
    Serial.print("Confirmed ");
    switch(cmd){ // switch statement allows for building of a variety of command interfaces
      case 114: // if the incoming byte is 'r'
        count = 0;
        Serial.print("count = 0\n");
        break;
      case 115: // if incoming byte is s immediately followed by a number
        stim = Serial.parseInt(); // parse the number into a frame
        Serial.print("stimulus is now on frame ");
        Serial.print(stim);
        Serial.print("\n");
        break;
      case 83:
        digitalWrite(outs[0],HIGH);
        Serial.print("STIM!\n");
        digitalWrite(outs[0],LOW); 
        break;
      default:
        Serial.print("instructions unclear you gave ");
        Serial.print(cmd);
        Serial.print("\n");
    }
  }
}

