/* 
 TTL-Olfactory-stimulator
 Author: Nicholas Mei
*/
 
// Fast Pin operations using port registers
// Code from: http://masteringarduino.blogspot.com/2013/10/fastest-and-smallest-digitalread-and.html
#define portOfPin(P)\
  (((P)>=0&&(P)<8)?&PORTD:(((P)>7&&(P)<14)?&PORTB:&PORTC)) // If Pin # is >= 0 and < 8 return PORTD address, otherwise if Pin # is greater than 7 and less than 14 return PORTB, otherwise return PORTC
#define ddrOfPin(P)\
  (((P)>=0&&(P)<8)?&DDRD:(((P)>7&&(P)<14)?&DDRB:&DDRC))
#define pinOfPin(P)\
  (((P)>=0&&(P)<8)?&PIND:(((P)>7&&(P)<14)?&PINB:&PINC))
#define pinIndex(P)((uint8_t)(P>13?P-14:P&7))
#define pinMask(P)((uint8_t)(1<<pinIndex(P)))

#define pinAsInput(P) *(ddrOfPin(P))&=~pinMask(P)
#define pinAsInputPullUp(P) *(ddrOfPin(P))&=~pinMask(P);digitalHigh(P)
#define pinAsOutput(P) *(ddrOfPin(P))|=pinMask(P)
#define digitalLow(P) *(portOfPin(P))&=~pinMask(P)
#define digitalHigh(P) *(portOfPin(P))|=pinMask(P)
#define isHigh(P)((*(pinOfPin(P))& pinMask(P))>0)
#define isLow(P)((*(pinOfPin(P))& pinMask(P))==0)
#define digitalState(P)((uint8_t)isHigh(P))

// Set pin numbers
const byte TTL_PIN = 12;               // Arduino will expect TTL pulse input on pin 12
const byte SOL_PIN_1 = 3;
const byte SOL_PIN_2 = 5;
const byte SOL_PIN_3 = 6;
const byte SOL_PIN_4 = 9;

// Note: using unsigned long will guarantee overflow proof code
// micros diff comparisons will be the equivalent to: (current_micros-ttl_time) % 4,294,967,295
unsigned long ttl_time = 0;            // variable to store when a TTL pulse is received by the Arduino
unsigned long solenoid_on_time = 0;
unsigned long current_micros = 0;
unsigned long micros_elapsed = 0;     

boolean solenoid_state = LOW;
boolean run_solenoid_timer = false;

/*============== User Defined Variables ==================*/
float des_sol_on_time = 1;      // Desired ON time for a solenoid (in seconds)
float des_post_ttl_delay = 0;     // Desired amount of time to wait after TTL pulse to turn on solenoid (in seconds)
float sol_cooldown = 1;         // After a bout of olfactory stimulation, this is the length of time (in seconds) to 'cooldown' before allowing activation of the solenoid again
int num_bouts = 6;               // Number of bouts of odor stimulation to deliver after receiving a TTL pulse (inter-bout interval time will be determined by 'solenoid_cooldown')

// Perform conversions from user defined variables (in seconds) to microseconds
const float desired_solenoid_on_time = des_sol_on_time * 1000000;
const float desired_post_ttl_delay = des_post_ttl_delay * 1000000;
const float solenoid_cooldown = sol_cooldown * 1000000;
const int num_stim_bouts = num_bouts;

/*===================== Setup ==============================*/
void setup() {  
  // Set the digital pins as inputs or output: 
  pinAsInput(TTL_PIN);
  pinAsOutput(SOL_PIN_1);
  pinAsOutput(SOL_PIN_2);
  pinAsOutput(SOL_PIN_3);
  pinAsOutput(SOL_PIN_4);

  // Start serial comms and set baud rate to 115200
  // Only needed for debugging
  //Serial.begin(115200);
}

/*===================== Olfactory Stimulator Timing Loop ========================= */
void loop() {

  // Check if we have recieved a TTL pulse
  if (isHigh(TTL_PIN) && run_solenoid_timer == false) {
    //Serial.print("Received a TTL!\n");
    run_solenoid_timer = true;
    ttl_time = micros();
    num_bouts = num_stim_bouts;
  }

// Debug code for when we don't have a TTL pulse generator
//  if(Serial.available() > 0) {
//    int val = Serial.read();
//    Serial.print("Received a TTL!\n");
//    run_solenoid_timer = true;
//    ttl_time = micros();
//    num_bouts = num_stim_bouts;
//  }
  
  // Check if we're in solenoid timer mode or not
  if (run_solenoid_timer == true) {
    current_micros = micros();
    micros_elapsed = current_micros - ttl_time;
    
    if (solenoid_state == LOW) {
      // Have we waited enough time after the TTL?
      if (micros_elapsed > desired_post_ttl_delay) {
        solenoid_state = HIGH;
        digitalHigh(SOL_PIN_1);
        //Serial.print("Solenoid is now HIGH\n");
        //Serial.println(micros());
        solenoid_on_time = micros();
      }
    }
    else {
      if (current_micros - solenoid_on_time > desired_solenoid_on_time) {
        solenoid_state = LOW;
        digitalLow(SOL_PIN_1);
        //Serial.print("Solenoid is now LOW");
        //Serial.println(micros());
        run_solenoid_timer = false;

        // After we turn off the solenoid (1 bout of olfactory stimulation), delay (blocks) until our cooldown has ellapsed
        // delay() function pauses program (in milleseconds)
        delay((unsigned long)(solenoid_cooldown/1000)); 

        if (num_bouts > 1) {
          run_solenoid_timer = true;
          ttl_time = micros();
          num_bouts--;
        }
      }
    }
  }
}
