#include <FS.h>  // Include the SPIFFS library

void setup() {
String strhtml="<html>";
 Serial.begin(115200); // Start the Serial communication to send messages to the computer
 delay(10);
 Serial.println('\n');

  bool fsMounted = SPIFFS.begin();
  if (fsMounted ) {
    Serial.println("File system mounted with success");
  } else {
    Serial.println("Error mounting the file system");
    return;
  }

  Dir dir = SPIFFS.openDir("");
  while (dir.next()) {
      Serial.print(dir.fileName());
      File f = dir.openFile("r");
      Serial.print(" ");
      Serial.print(f.size());
      Serial.println(" bytes");
  }
  Serial.println("===============================");

  File file = SPIFFS.open("/index.html", "r"); // @suppress("Abstract class cannot be instantiated")
  if (!file) {
    Serial.println("Error opening file");
    return;
  }
  while(file.available()) {
  	String line = file.readStringUntil('\n');
    Serial.println(line);
  }
    file.close();
    Serial.println("File closed");
}




void loop() {}

