// Check System Endianness in C/C++

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
}

void loop() {
  /*Check System Endianness*/
  int n = 1;
  // little endian if true
  if(*(char *)&n == 1) {
    Serial.println(true);
  } else {
    Serial.println(false);
  }

  // if 1 means the system is Little Endian
}
