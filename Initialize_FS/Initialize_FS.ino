#include <FS.h>  // Include the SPIFFS library

void setup() {
String strhtml="<! DOCTYPE html>< html><head><meta http-equiv=\"Content-Language\" content=\"en-ca\"><meta name=\"viewport\" content=\"width=device-width, user-scalable=no\"head>" ;
  	   strhtml = strhtml + "<body><form action=\"save\" method=\"post\">";
  	   strhtml = strhtml + "<table border=\"0\" width=\"400\" cellspacing=\"0\" cellpadding=\"0\">";
  	   strhtml = strhtml + "<tr><td colspan=\"2\" bgcolor=\"#000000\"><p align=\"center\"><font color=\"#FFFFFF\"><b><font size=\"5\">SDLogik</font><br>Network Setup</b></font></td></tr>";
	   strhtml = strhtml + "<tr><td width=\"148\">&nbsp;</td><td width=\"188\">&nbsp;</td></tr>" ;
	   strhtml = strhtml + "<tr><td width=\"148\"><font color=\"#FF0000\">&nbsp;&nbsp;*</font>SSID </td><td width=\"188\"><input type=\"text\" name=\"SSID\" id=\"SSID\" required size=\"20\"></td></tr>" ;
	   strhtml = strhtml + "<tr><td width=\"148\"><font color=\"#FF0000\">&nbsp;&nbsp;*</font>PWD </td><td width=\"188\"><input type=\"password\" name=\"pwd\" id=\"pwd\" required></td></tr>" ;
  	   strhtml = strhtml + "<tr><td width=\"148\">&nbsp;&nbsp;MQTT ID</td><td width=\"188\"><input type=\"text\" name=\"mqttID\" id=\"mqttID\" ></td></tr>" ;
	   strhtml = strhtml + "<tr><td width=\"148\">&nbsp;&nbsp;MQTT IP</td><td width=\"188\"><input type=\"text\" name=\"mqttIP\" id=\"mqttIP\" ></td></tr>" ;
	   strhtml = strhtml + "<tr><td width=\"148\">&nbsp;&nbsp;MQTT Port </td><td width=\"188\"><input type=\"text\" name=\"mqttPort\" id=\"mqttPort\" size=\"6\" �placeholder=\"1883\"></td></tr>" ;
	   strhtml = strhtml + "<tr><td width=\"148\">&nbsp;&nbsp;MQTT User</td><td width=\"188\"><input type=\"text\" name=\"mqttUser\" id=\"mqttUser\"></td></tr>" ;
	   strhtml = strhtml + "<tr><td width=\"148\">&nbsp;&nbsp;MQTT Pwd</td><td width=\"188\"><input type=\"password\" name=\"mqttPwd\" id=\"mqttPwd\" size=\"20\"></td></tr>" ;
	   strhtml = strhtml + "<tr><td width=\"148\">&nbsp;&nbsp;Device IP</td><td width=\"188\"><select size=\"1\" name=\"DeviceIP\"><option selected>DHCP</option><option>Static</option></select></td></tr>" ;
	   strhtml = strhtml + "<tr><td width=\"148\">&nbsp;&nbsp;Static IP</td><td width=\"188\"><input type=\"text\" name=\"staticIP\" id=\"staticIP\"></td></tr>" ;
	   strhtml = strhtml + "<tr><td width=\"148\">&nbsp;&nbsp;Gateway</td><td width=\"188\"><input type=\"text\" name=\"gateway\" id=\"gateway\"></td></tr>" ;
	   strhtml = strhtml + "<tr><td width=\"148\">&nbsp;&nbsp;Subnet</td><td width=\"188\"><input type=\"text\" name=\"subnet\" id=\"subnet\"></td></tr>" ;
	   strhtml = strhtml + "<tr><td width=\"148\">&nbsp;&nbsp;DNS</td><td width=\"188\"><input type=\"text\" name=\"DNS\" id=\"DNS\"></td></tr>" ;
	   strhtml = strhtml + "<tr><td width=\"148\">&nbsp;&nbsp;Syslog Enabled</td><td width=\"188\"><input type=\"checkbox\" name=\"syslog\" value=\"ON\"></td></tr>" ;
	   strhtml = strhtml + "<tr><td width=\"148\">&nbsp;&nbsp;Syslog Server</td><td width=\"188\"><input type=\"text\" name=\"syslogServer\" id=\"syslogServer\"></td></tr>" ;
	   strhtml = strhtml + "<tr><td width=\"148\">&nbsp;&nbsp;Syslog Port</td><td width=\"188\"><input type=\"text\" name=\"syslogPort\" id=\"syslogPort\" size=\"6\"�placeholder=\"514\"></td></tr>" ;
	   strhtml = strhtml + "<tr><td width=\"148\">&nbsp;&nbsp;Enable API</td><td width=\"188\"><input type=\"checkbox\" name=\"API\" value=\"ON\"></td></tr>" ;
	   strhtml = strhtml + "<tr><td width=\"148\">&nbsp;&nbsp;Remote Config</td><td width=\"188\"><input type=\"text\" name=\"remoteConfig\" id=\"remoteConfig\" size=\"25\"></td></tr>" ;
	   strhtml = strhtml + "<tr><td width=\"336\" colspan=\"2\"><p align=\"left\">&nbsp;</td></tr>" ;
	   strhtml = strhtml + "<tr><td width=\"336\" colspan=\"2\"><p align=\"center\"><input type=\"submit\" name=\"save\" value=\"Save Config\">&nbsp;&nbsp;&nbsp;" ;
	   strhtml = strhtml + "<input type=\"submit\" name=\"reset\" value=\"Reset Form\"></td></tr>" ;
	   strhtml = strhtml + "</table><p align=\"left\"><font color=\"#FF0000\">*</font> Required</p></form></body></html>";

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
  File file = SPIFFS.open("/index.html", "w"); // @suppress("Abstract class cannot be instantiated")
  if (!file) {
    Serial.println("Error opening file for writing");
    return;
  }
  int bytesWritten = file.print(strhtml);

  if (bytesWritten > 0) {
    Serial.print("File /index.html was written with ");
    Serial.print(bytesWritten);
    Serial.println(" Bytes");
  } else {
    Serial.println("File write failed");
  }
  file.close();
}

void loop() {}
