void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
}


int rpm = 700;
int speed = 10;
int throttle = 40;
void loop() {
  // put your main code here, to run repeatedly:
  Serial.print("S");
  Serial.print(rpm);
  Serial.print(",");
  Serial.print(speed);
  Serial.print(",");
  Serial.print(throttle);
  Serial.print("E");

  rpm += random(-50, 50);
  speed += random(-5, 5);
  throttle += random(-10, 10);

  if(rpm < 0) rpm = 0;
  if(rpm > 5000) rpm = 5000;
  if(speed < 0) speed = 0;
  if(speed > 80) speed = 80;
  if(throttle < 0) throttle = 0;
  if(throttle > 100) throttle = 100;
  delay(300);
}
