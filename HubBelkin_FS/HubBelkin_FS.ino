#include <FS.h> // Include the SPIFFS library

//===================================================================================
// Setup Routine
//===================================================================================
void setup() {

 Serial.begin(19200); // Start the Serial communication to send messages to the computer
 delay(10);
 Serial.println('\n');

  bool fsMounted = SPIFFS.begin();
  if (fsMounted) {
    Serial.println("File system mounted with success");
  } else {
    Serial.println("Error mounting the file system");
    return;
  }
  CreateSetupPage();
}


//===================================================================================
// Create Setup Page
//===================================================================================
void CreateSetupPage() {
String strhtml="<html><head><meta http-equiv=\"Content-Language\" content=\"en-ca\"><meta name=\"viewport\" content=\"width=device-width, user-scalable=no\"head>" ;
   strhtml += "<body><form action=\"/adddevice\" method=\"post\">";
   strhtml += "<table border=\"0\" width=\"400\" cellspacing=\"0\" cellpadding=\"0\"><tr><td colspan=\"5\" bgcolor=\"#000000\"><p align=\"center\"><font color=\"#FFFFFF\"><b><font size=\"5\">Hub Belkin</font><br>Device Setup</b></font></td></tr>";
   strhtml += "<tr><td width=\"100\" height=\"24\" bgcolor=\"#C0C0C0\">Name</td><td width=\"100\" height=\"24\" bgcolor=\"#C0C0C0\">Type</td><td width=\"134\" height=\"24\" bgcolor=\"#C0C0C0\">IP</td><td width=\"91\" height=\"24\" bgcolor=\"#C0C0C0\" colspan=\"2\">Port</td></tr>";
   strhtml += ">><<";
   strhtml += "<tr><td width=\"100\"><input type=\"text\" name=\"Name\" id=\"Name\" required size=\"10\"></td>";
   strhtml += "<td width=\"100\"><select size=\"1\" name=\"type\"><option selected>Select</option><option>Switch</option><option>Dimmer</option><option>Plug</option></select></td>";
   strhtml += "<td width=\"134\"><input type=\"text\" name=\"IP\" id=\"IP\" required size=\"10\"></td>	<td width=\"91\" colspan=\"2\"><input type=\"text\" name=\"Port\" id=\"Port\" required size=\"5\"></td></tr>";
   strhtml += "<tr><td width=\"309\" colspan=\"3\"><p align=\"left\"><input type=\"submit\" name=\"saveDevice\" value=\"Add device\"></td></tr>";
   strhtml += "</table></form></body></html>";

 File file = SPIFFS.open("/devices.html", "w"); // @suppress("Abstract class cannot be instantiated")
 if (!file) {
  Serial.println("Error opening file /devices.html for writing");
  return;
 }
 int bytesWritten = file.print(strhtml);

 if (bytesWritten > 0) {
  Serial.print("File /devices.html was written with ");
  Serial.print(bytesWritten);
  Serial.println(" Bytes");
 } else {
  Serial.println("File write failed (/devices.html)");
 }
 file.close();
 delay(10);
}



//===================================================================================
// Main Loop
//===================================================================================
void loop() {}
