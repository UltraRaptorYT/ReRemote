#include <IRremote.h>

IRsend irsend;

void setup() {
  Serial.begin(115200);
}

void loop() {
  for(int i = 0; i < 3; i++) {
    irsend.sendNEC(0x4655434b, 32);
    Serial.println("SENDING");
  }
  delay(10);
}
