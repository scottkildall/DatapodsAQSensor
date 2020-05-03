/*
  DatapodsAQSensor
  by Scott Kildall
  
  Uses the Plantower AQ sensor
  Routes the PM 2.5 and PM 10 results to the analog output pins for
  the Datapods to use as their self-contained input

  We use a hard delay() in the loop, as the asynchronous timer tends to
  slow things down. At 500ms, we still get "no data" for every 30% of the calls,
  so this number is okay.

  We can always see serial ouput without a significant penality to the code performance.
 */

#include <SoftwareSerial.h>

SoftwareSerial pmsSerial(27, 33);

#define pm25Pin (25)
#define pm100Pin (26)

#define MAX_xxx

// PM values, for output to DAC
uint16_t  pm100Val;
uint16_t  pm25Val;

void setup() {
  // our debugging output
  Serial.begin(115200);
  Serial.println("Plantower starting up");

  pinMode(pm25Pin,OUTPUT);
  pinMode(pm100Pin,OUTPUT);
  
  // sensor baud rate is 9600
  pmsSerial.begin(9600);

  pm25Val = 0;
  pm100Val = 0;
}

struct pms5003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm10_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
};

struct pms5003data data;
    
void loop() {   
  if (readPMSdata(&pmsSerial)) {
    pm25Val = data.pm25_standard;
    pm100Val = data.pm100_standard;

    //serialOutFullRead();
    serialOutSmallRead();
    outputPMValues();
  }
  else
    Serial.println("no data");

  // 100 ms / read, data reads happen more slowly in most caases
  delay(500);
}

boolean readPMSdata(Stream *s) {
  if (! s->available()) {
    return false;
  }
  
  // Read a byte at a time until we get to the special '0x42' start-byte
  if (s->peek() != 0x42) {
    s->read();
    return false;
  }

  // Now read all 32 bytes
  if (s->available() < 32) {
    return false;
  }
    
  uint8_t buffer[32];    
  uint16_t sum = 0;
  s->readBytes(buffer, 32);

  // get checksum ready
  for (uint8_t i=0; i<30; i++) {
    sum += buffer[i];
  }

  /* debugging
  for (uint8_t i=2; i<32; i++) {
    Serial.print("0x"); Serial.print(buffer[i], HEX); Serial.print(", ");
  }
  Serial.println();
  */
  
  // The data comes in endian'd, this solves it so it works on all platforms
  uint16_t buffer_u16[15];
  for (uint8_t i=0; i<15; i++) {
    buffer_u16[i] = buffer[2 + i*2 + 1];
    buffer_u16[i] += (buffer[2 + i*2] << 8);
  }

  // put it into a nice struct :)
  memcpy((void *)&data, (void *)buffer_u16, 30);

  if (sum != data.checksum) {
    Serial.println("Checksum failure");
    return false;
  }
  // success!
  return true;
}

void outputPMValues() {
  // 8-bit values

  float fp25 = (float)pm25Val/2.0;    
  float fp10 = (float)pm100Val/2.0;   


  // DAC maximum is 255 
  // you won't normally go this high unless directly in a fire or something like that
  if( fp25 > 255.0 )
    fp25 = 255.0;
  
  if( fp10 > 255.0 )
    fp10 = 255.0;
 
   dacWrite(pm25Pin, fp25);
   dacWrite(pm100Pin, fp10);
}

void serialOutSmallRead() {
    Serial.println("-----");
    Serial.print("PM 2.5: ");
    Serial.print(data.pm25_standard);
    Serial.print("\t\tPM 10: ");
    Serial.println(data.pm100_standard);
} 

void serialOutFullRead() { 
  // reading data was successful!
    Serial.println();
    Serial.println("---------------------------------------");
    Serial.println("Concentration Units (standard)");
    Serial.print("PM 1.0: "); Serial.print(data.pm10_standard);
    Serial.print("\t\tPM 2.5: "); Serial.print(data.pm25_standard);
    Serial.print("\t\tPM 10: "); Serial.println(data.pm100_standard);
    Serial.println("---------------------------------------");
    Serial.println("Concentration Units (environmental)");
    Serial.print("PM 1.0: "); Serial.print(data.pm10_env);
    Serial.print("\t\tPM 2.5: "); Serial.print(data.pm25_env);
    Serial.print("\t\tPM 10: "); Serial.println(data.pm100_env);
    Serial.println("---------------------------------------");
    Serial.print("Particles > 0.3um / 0.1L air:"); Serial.println(data.particles_03um);
    Serial.print("Particles > 0.5um / 0.1L air:"); Serial.println(data.particles_05um);
    Serial.print("Particles > 1.0um / 0.1L air:"); Serial.println(data.particles_10um);
    Serial.print("Particles > 2.5um / 0.1L air:"); Serial.println(data.particles_25um);
    Serial.print("Particles > 5.0um / 0.1L air:"); Serial.println(data.particles_50um);
    Serial.print("Particles > 10.0 um / 0.1L air:"); Serial.println(data.particles_100um);
    Serial.println("---------------------------------------");
}
