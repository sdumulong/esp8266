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
  CreateSuccessPage();
  CreateFailedPage();
}


//===================================================================================
// Create Setup Page
//===================================================================================
void CreateSetupPage() {
String strhtml="<html><head><meta http-equiv=\"Content-Language\" content=\"en-ca\"><meta name=\"viewport\" content=\"width=device-width, user-scalable=no\"head>" ;
   strhtml += "<body><form action=\"save\" method=\"post\">";
   strhtml += "<table border=\"0\" width=\"400\" cellspacing=\"0\" cellpadding=\"0\">";
   strhtml += "<tr><td colspan=\"2\" bgcolor=\"#000000\"><p align=\"center\"><font color=\"#FFFFFF\"><b><font size=\"5\">SDLogik</font><br>Network Setup</b></font></td></tr>";
   strhtml += "<tr><td width=\"148\">&nbsp;</td><td width=\"188\">&nbsp;</td></tr>";
   strhtml += "<tr><td width=\"148\"><font color=\"#FF0000\">&nbsp;&nbsp;*</font>SSID </td><td width=\"188\"><input type=\"text\" name=\"SSID\" id=\"SSID\" required size=\"20\"></td></tr>";
   strhtml += "<tr><td width=\"148\"><font color=\"#FF0000\">&nbsp;&nbsp;*</font>PWD </td><td width=\"188\"><input type=\"password\" name=\"pwd\" id=\"pwd\" required></td></tr>";
   strhtml += "<tr><td width=\"148\"><font color=\"#FF0000\">&nbsp;&nbsp;*</font>Device ID</td><td width=\"188\"><input type=\"text\" name=\"mqttID\" id=\"mqttID\" required></td></tr>";
   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;Device IP</td><td width=\"188\"><select size=\"1\" name=\"DeviceIP\"><option selected>DHCP</option><option>Static</option></select></td></tr>";
   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;MQTT Enabled</td><td width=\"188\"><input type=\"checkbox\" name=\"MQTT\" value=\"ON\"></td></tr>";
   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;MQTT IP</td><td width=\"188\"><input type=\"text\" name=\"mqttIP\" id=\"mqttIP\" size=\"12\"></td></tr>";
   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;MQTT Port </td><td width=\"188\"><input type=\"text\" name=\"mqttPort\" id=\"mqttPort\" size=\"6\" placeholder=\"1883\"></td></tr>";
   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;MQTT User</td><td width=\"188\"><input type=\"text\" name=\"mqttUser\" id=\"mqttUser\"></td></tr>";
   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;MQTT Pwd</td><td width=\"188\"><input type=\"password\" name=\"mqttPwd\" id=\"mqttPwd\" size=\"20\"></td></tr>";
   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;Static IP</td><td width=\"188\"><input type=\"text\" name=\"staticIP\" id=\"staticIP\" size=\"12\"></td></tr>";
   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;Gateway</td><td width=\"188\"><input type=\"text\" name=\"gateway\" id=\"gateway\" size=\"12\"></td></tr>";
   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;Subnet</td><td width=\"188\"><input type=\"text\" name=\"subnet\" id=\"subnet\" size=\"12\"></td></tr>";
   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;DNS</td><td width=\"188\"><input type=\"text\" name=\"DNS\" id=\"DNS\" size=\"12\"></td></tr>";
   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;MAC</td><td width=\"188\"><input type=\"text\" name=\"MAC\" id=\"MAC\"></td></tr>";
   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;Enable UPnP</td><td width=\"188\"><input type=\"checkbox\" name=\"UPnP\" value=\"ON\"></td></tr>";
   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;Syslog Enabled</td><td width=\"188\"><input type=\"checkbox\" name=\"syslog\" value=\"ON\"></td></tr>";
   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;Syslog Server</td><td width=\"188\"><input type=\"text\" name=\"syslogServer\" id=\"syslogServer\"></td></tr>";
   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;Syslog Port</td><td width=\"188\"><input type=\"text\" name=\"syslogPort\" id=\"syslogPort\" size=\"6\" placeholder=\"514\"></td></tr>";
   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;Enable API</td><td width=\"188\"><input type=\"checkbox\" name=\"API\" value=\"ON\"></td></tr>";
   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;Enable SSL</td><td width=\"188\"><input type=\"checkbox\" name=\"SSL\" value=\"ON\"></td></tr>";
   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;Certificate Path</td><td width=\"188\"><input type=\"text\" name=\"CertPath\" size=\"25\"></td></tr>";
   strhtml += "<tr><td width=\"336\" colspan=\"2\"><p align=\"left\">&nbsp;</td></tr>";
   strhtml += "<tr><td width=\"336\" colspan=\"2\"><p align=\"center\"><input type=\"submit\" name=\"save\" value=\"Save Config\">&nbsp;&nbsp;&nbsp;";
   strhtml += "<input type=\"submit\" name=\"reset\" value=\"Reset Form\"></td></tr>";
   strhtml += "</table><p align=\"left\"><font color=\"#FF0000\">*</font> Required</p></form></body></html>";

 File file = SPIFFS.open("/index.html", "w"); // @suppress("Abstract class cannot be instantiated")
 if (!file) {
  Serial.println("Error opening file /index.html for writing");
  return;
 }
 int bytesWritten = file.print(strhtml);

 if (bytesWritten > 0) {
  Serial.print("File /index.html was written with ");
  Serial.print(bytesWritten);
  Serial.println(" Bytes");
 } else {
  Serial.println("File write failed (/Index.html)");
 }
 file.close();
 delay(10);
}


//===================================================================================
// Create Success Page
//===================================================================================
void CreateSuccessPage() {
String strhtml="<html><head><meta http-equiv=\"Content-Language\" content=\"en-ca\"><meta name=\"viewport\" content=\"width=device-width, user-scalable=no\"head>" ;
   strhtml = strhtml + "<body><table border=\"0\" width=\"400\" cellspacing=\"0\" cellpadding=\"0\">";
   strhtml = strhtml + "<tr><td colspan=\"2\" bgcolor=\"#000000\"><p align=\"center\"><font color=\"#FFFFFF\"><b><font size=\"5\">SDLogik</font><br>Network Setup</b></font></td></tr>";
 strhtml = strhtml + "<tr><td width=\"148\">&nbsp;</td><td width=\"188\">&nbsp;</td></tr>";
 strhtml = strhtml + "</table><p align=\"left\"><font color=\"#00FF00\"><br>Configuration has been successfully saved</font></p></form></body></html>";

 File file = SPIFFS.open("/success.html", "w"); // @suppress("Abstract class cannot be instantiated")
 if (!file) {
  Serial.println("Error opening file /success.html for writing");
  return;
 }
 int bytesWritten = file.print(strhtml);

 if (bytesWritten > 0) {
  Serial.print("File /success.html was written with ");
  Serial.print(bytesWritten);
  Serial.println(" Bytes");
 } else {
  Serial.println("File write failed (/success.html)");
 }
 file.close();
 delay(10);
}


//===================================================================================
// Create Failed page
//===================================================================================
void CreateFailedPage() {
String strhtml="<html><head><meta http-equiv=\"Content-Language\" content=\"en-ca\"><meta name=\"viewport\" content=\"width=device-width, user-scalable=no\"head>" ;
   strhtml = strhtml + "<body><table border=\"0\" width=\"400\" cellspacing=\"0\" cellpadding=\"0\">";
   strhtml = strhtml + "<tr><td colspan=\"2\" bgcolor=\"#000000\"><p align=\"center\"><font color=\"#FFFFFF\"><b><font size=\"5\">SDLogik</font><br>Network Setup</b></font></td></tr>";
 strhtml = strhtml + "<tr><td width=\"148\">&nbsp;</td><td width=\"188\">&nbsp;</td></tr>";
 strhtml = strhtml + "</table><p align=\"left\"><font color=\"#FF0000\"><br>Failed to save configuration !</font></p></body></html>";

 File file = SPIFFS.open("/failed.html", "w"); // @suppress("Abstract class cannot be instantiated")
 if (!file) {
  Serial.println("Error opening file /failed.html for writing");
  return;
 }
 int bytesWritten = file.print(strhtml);

 if (bytesWritten > 0) {
  Serial.print("File /failed.html was written with ");
  Serial.print(bytesWritten);
  Serial.println(" Bytes");
 } else {
  Serial.println("File write failed (/failed.html)");
 }
 file.close();
 delay(10);
}


//===================================================================================
// Main Loop
//===================================================================================
void loop() {}
