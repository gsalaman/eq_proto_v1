/* Stepper experiments for the proto CS EQ mount */

#define STEP_PIN   2
#define DIR_PIN    3
#define MS1_PIN    4
#define MS2_PIN    5  
#define ENABLE_PIN 6
#define MS3_PIN    7  // should really put this next to MS1 and MS2, but I want to make it just
                      // an addition to the previous "easy driver".

int current_dir = HIGH;
uint32_t delay_ms=10;

typedef enum
{
  STATE_STOPPED,
  STATE_MOVE_FAST,
  STATE_MOVE_SLOW
} eq_state_type;

eq_state_type current_state=STATE_STOPPED;

void set_fast_motion( void )
{
  digitalWrite(ENABLE_PIN, LOW);
  Serial.println("Setting Motion to FAST");
  current_state = STATE_MOVE_FAST;
}

void set_slow_motion( void )
{
  digitalWrite(ENABLE_PIN, LOW);
  Serial.println("Setting motion to SLOW");
  current_state = STATE_MOVE_SLOW;
}

void stop_motion( void )
{
  digitalWrite(ENABLE_PIN, HIGH);
  Serial.println("Stopping");
  current_state = STATE_STOPPED;
}

void switch_direction( void )
{
  Serial.println("Switching direction");
  
  if (current_dir == HIGH) current_dir = LOW;
  else current_dir = HIGH;

  digitalWrite(DIR_PIN, current_dir);
  
}

void set_ms_delay( uint32_t ms)
{

  if (ms == 0)
  {
    Serial.println("Zero delay not allowed");
    return;
  }
  
  Serial.print("Setting delay to ");
  Serial.print(ms);
  Serial.println(" ms");

  delay_ms = ms;  
}

void print_menu( void )
{
  Serial.println("f = fast motion");
  Serial.println("s = slow motion");
  Serial.println("d = switch direction");
  Serial.println("x = stop");
  Serial.println("number to set ms delay");  
}

void process_serial( void )
{
  char c;
  static bool building_number=false;
  static uint32_t  number = 0;
  
  /* commands:
   *  f = fast motion
   *  d = switch direction
   *  x = stop
   *  s = slow motion
   *  any number sets the microsecond delay
   */
  if (Serial.available())
  {
    c = Serial.read();

    if (c >= '0' && c <= '9')
    {
      building_number = true;
      
      /* continue building our number input */
      number = number * 10;
      number = number + (c - '0');   
    }

    else
    {
        
      switch (c)
      {
        case '\n':
          if (building_number)
          {
            set_ms_delay(number);
        
            number = 0;
            building_number=false;
          }
        break;
  
        case 'f':
          set_fast_motion();
        break;
  
        case 'd':
          switch_direction();
        break;
  
        case 's':
          set_slow_motion();
        break;
  
        case 'x':
          stop_motion();
        break;
  
        default:
          Serial.print("Unknown input: ");
          Serial.println(c);
          print_menu();
      }
    }
  }
}

void process_fast_state( void )
{
  /* Hack version with embedded waits */

  // Make sure we're doing full steps
  digitalWrite(MS1_PIN, LOW);
  digitalWrite(MS2_PIN, LOW);
  digitalWrite(MS3_PIN, LOW);
  
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(500);
  digitalWrite(STEP_PIN,LOW);
  delayMicroseconds(500);
}

void process_slow_state( void )
{
  uint32_t half_delay_us;

  
  
  half_delay_us = 1000 * delay_ms / 2;
  //Serial.println(half_delay_us);
  
  // Make sure we're doing 1/16 steps
  digitalWrite(MS1_PIN, HIGH);
  digitalWrite(MS2_PIN, HIGH);
  digitalWrite(MS3_PIN, HIGH);

  // We're gonna split the delay in half to make a square wave.  
  // Unfortunately, delayMicroseconds only works up to about 16000 (16 ms). 
  // To get around this, if our timer is less than 30 ms, we'll divide it into microsecond chunks.
  // otherwise, we'll just use milliseconds.
  if (delay_ms > 30)
  { 
    digitalWrite(STEP_PIN, HIGH);
    delay(delay_ms / 2);
    digitalWrite(STEP_PIN,LOW);
    delay( (delay_ms / 2) + delay_ms % 2);
  }
  else
  {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(half_delay_us);
    digitalWrite(STEP_PIN,LOW);
    delayMicroseconds(half_delay_us);  
  }
}

void process_stopped_state( void )
{

  
}

void state_machine_driver( void )
{
  switch (current_state)
  {
    case STATE_MOVE_FAST:
      process_fast_state();
    break;

    case STATE_MOVE_SLOW:
      process_slow_state();
    break;

    case STATE_STOPPED:
      process_stopped_state();
    break;

    default:
      Serial.print("UNKNOWN STATE: ");
      Serial.println(current_state);
  }
}

void setup() 
{
  Serial.begin(9600);

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(MS1_PIN, OUTPUT);
  pinMode(MS2_PIN, OUTPUT);
  pinMode(MS3_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);

  digitalWrite(STEP_PIN, LOW);

  digitalWrite(ENABLE_PIN, HIGH);  // Disabled.  We'll turn it on when commanded.

  digitalWrite(DIR_PIN, current_dir);

  /* Truth table for MS selects:
   *  MS1  MS2  MS3   Step 
   *  L    L    L     Full
   *  H    L    L     Half
   *  L    H    L     Quarter
   *  H    H    L     Eighth
   *  H    H    H     Sixteenth
   */
  digitalWrite(MS1_PIN, HIGH);
  digitalWrite(MS2_PIN, HIGH);
  digitalWrite(MS3_PIN, HIGH);

  Serial.println("inited");
}

void loop() 
{

  process_serial();

  state_machine_driver();
  
}
