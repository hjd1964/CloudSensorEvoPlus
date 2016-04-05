#if defined(W5100_ON)

// The Ethernet channel code below was written by Luka
// you can telnet into it at port 9999
// provides the same functions as for serial

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:

// for ethernet

// Network server port for command channel
EthernetServer cmd_server(9999);
EthernetClient cmd_client;
// Network server port for web channel
EthernetServer web_server(80);
EthernetClient www_client;
boolean alreadyConnected = false; // whether or not the client was connected previously
long cmdTransactionLast_ms = 0;   // timed stop()

void Ethernet_Init() {
  // initialize the ethernet device
#if defined(ETHERNET_USE_DHCP_ON)
  Ethernet.begin(mac);
#else
  Ethernet.begin(mac, ip, myDns, gateway, subnet);
#endif

  cmd_server.begin();
  web_server.begin();

  cmdTransactionLast_ms=millis();
}

bool Ethernet_www_busy() {
  return www_client;
}

bool Ethernet_cmd_busy() {
  return cmd_client;
}

void Ethernet_send(const char data[]) {
  if (!cmd_client) return;
  cmd_client.flush();
  cmd_client.write(data,strlen(data));
  cmdTransactionLast_ms=millis();
}

void Ethernet_print(const char data[]) {
  Ethernet_send(data);
}
 
boolean Ethernet_transmit() {
  return false;
}
 
boolean Ethernet_available() {
  cmd_client = cmd_server.available();
  if (cmd_client) {
    if (cmd_client.connected()) return cmd_client.available();
    if (millis()-cmdTransactionLast_ms>2000) cmd_client.stop();
  }
  return false;
}
 
char Ethernet_read() {
  return cmd_client.read();
}

// -----------------------------------------------------------------------------------------
// web server

boolean currentLineIsBlank;
boolean responseStarted = false;
boolean clientNeedsToClose = false;
unsigned long responseFinish_ms;
unsigned long transactionStart_ms;
int html_page_step=0;

#ifdef SD_CARD_ON
const char modifiedSince[] = "If-Modified-Since:"; bool modifiedSince_found; byte modifiedSince_count;
#endif

// variables to support web-page request detection
const char index_page[] = "GET /index.htm"; bool index_page_found; byte index_page_count;
#ifdef SD_CARD_ON
const char chart_js_page[] = "GET /Chart.min.js"; bool chart_js_page_found; byte chart_js_page_count;
#endif
/* add additional web-pages per the above line(s) here */

void reset_page_requests() {
  index_page_found=false; index_page_count=0;
#ifdef SD_CARD_ON
  chart_js_page_found=false; chart_js_page_count=0;
#endif
/* add additional web-pages per the above line(s) here */

#ifdef SD_CARD_ON
  modifiedSince_found=false;
#endif
}

void Ethernet_www() {
  // if a client doesn't already exist try to find a new one
  if (!www_client) { www_client = web_server.available(); currentLineIsBlank = true; responseStarted=false; clientNeedsToClose=false; reset_page_requests(); transactionStart_ms=millis(); }

  // active client?
  if (www_client) {
    if (www_client.connected()) {
      if (www_client.available() && (!responseStarted)) {
        char c = www_client.read();
       // Serial.print(c);
        
        // watch for page requests and flag them
        if (!index_page_found)    { if (c==index_page[index_page_count])       index_page_count++;    else index_page_count=0;    if (index_page_count==14)    { index_page_found=true; } }
#ifdef SD_CARD_ON
        if (!chart_js_page_found) { if (c==chart_js_page[chart_js_page_count]) chart_js_page_count++; else chart_js_page_count=0; if (chart_js_page_count==17) { chart_js_page_found=true; } }
#endif
/* add additional web-pages per the above line(s) here */

#ifdef SD_CARD_ON
        // watch for cache requests
        if (!modifiedSince_found) { if (c==modifiedSince[modifiedSince_count]) modifiedSince_count++; else modifiedSince_count=0; if (modifiedSince_count==18) { modifiedSince_found=true; } }
#endif
        
        // if you've gotten to the end of the line (received a newline character) and the line is blank, the http request has ended, so you can send a reply
        if ((c == '\n') && currentLineIsBlank) { 
          responseStarted=true;
          html_page_step=0;
        } else {
          if (c == '\n') {
            // you're starting a new line
            currentLineIsBlank = true;
          } else if (c != '\r') {
            // you've gotten a character on the current line
            currentLineIsBlank = false;
          }
        }
      }
      // any data ready to send gets processed here
      if (responseStarted) {
        if (index_page_found) index_html_page(); else
#ifdef SD_CARD_ON
        if (chart_js_page_found) chart_js_html_page(); else
#endif
/* add additional web-pages per the above line(s) here */

        index_html_page();
      }
    }
    // if data was sent give the web browser time to receive it then stop the client
    // if a transaction is taking more than five seconds, stop the client
    if ((clientNeedsToClose && (millis()-responseFinish_ms>100)) || (millis()-transactionStart_ms>5000)) {
      clientNeedsToClose=false;
      www_client.stop();
    }
#ifdef ETHERNET_USE_DHCP_ON
    Ethernet.maintain();
#endif
  }
}

// --------------------------------------------------------------------------------------------------------------
// Web-site pages, the design of each xxxxx_html_page() function is now capable of sending a web-page of any size
// since it stops and waits should the local outgoing buffer become full
const char html_header12345[] PROGMEM = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\nRefresh: 10\r\n\r\n<!DOCTYPE HTML>\r\n<html>";
#ifdef SD_CARD_ON
const char html_header6[] PROGMEM = "<head><script src=\"/Chart.min.js\"></script>\r\n";
#else
//const char html_header6[] PROGMEM = "<head><script src=\"https://cdnjs.com/libraries/chart.js\"></script>";
const char html_header6[] PROGMEM = "<head><script src=\"http://www.chartjs.org/assets/Chart.min.js\"></script>";
#endif
const char html_header78[] PROGMEM = "</head><body bgcolor=\"#222\">";

const char html_main_css123[] PROGMEM = "<style>.t{padding:15px;border:15px solid #511;";
const char html_main_css4[] PROGMEM = "margin:25px;color:#999;background-color:#111;}.b{padding:30px;border:2px solid #511;";
const char html_main_css5[] PROGMEM = "margin:30px;color:#999;background-color:#111;}h1{text-align:right;}b{color:#922;}";
const char html_main_css6[] PROGMEM = ".u{background-color:#322;color:#a77;border:1px solid red;padding:5px 10px;}";
const char html_main_css7[] PROGMEM = ".g{background-color:#232;color:#7a7;border:1px solid green;padding:5px 10px;}";
const char html_main_css8[] PROGMEM = "input{width:4em}</style>";

const char html_links1[] PROGMEM = "<a href=\"/index.htm\">Status</a><br>";

// The index.htm page --------------------------------------------------------------------------------------
const char html_index1[] PROGMEM = "<div class=\"t\"><table width=\"100%\"><tr><td><strong>" FirmwareName " " FirmwareNumber;
const char html_index2[] PROGMEM = "</strong></td><td align=\"right\"><strong><font size=\"5\">";
const char html_index2a[] PROGMEM = "STATUS</font></strong></td></tr></table><br>";
const char html_index2b[] PROGMEM = "</div><div class=\"b\">";
const char html_index3[] PROGMEM = "Temperature:<br>Ambient <b>%s&deg;C</b> (Red)<br>";
const char html_index4[] PROGMEM = "Sky <b>%s&deg;C</b> (Blue)<br>";
#ifdef PlotAvgDeltaTemp_ON
const char html_index5[] PROGMEM = "&Delta; <b>%s&deg;C</b> (%s)<br>";
const char html_index6[] PROGMEM = "Avg. &Delta; <b>%s&deg;C</b> (Gray)";
#else
const char html_index5[] PROGMEM = "&Delta; <b>%s&deg;C</b> (<b>%s</b>, Gray)<br>";
const char html_index6[] PROGMEM = "Avg. &Delta; <b>%s&deg;C</b>";
#endif
const char html_index7[] PROGMEM = "Rain sense %s: <b>%s</b> ";
const char html_index8[] PROGMEM = "(%s)<br><br>";
#ifdef BMP180_ON
const char html_index9[] PROGMEM = "Humidity: <b>%s%%</b>, ";
#else
const char html_index9[] PROGMEM = "Humidity: <b>%s%%</b><br>";
#endif
const char html_index10[] PROGMEM = "Pressure: <b>%s</b> mbar <b>%s</b><br>";

const char clouds1[] PROGMEM = "Mainly Clear";
const char clouds2[] PROGMEM = "Few or High Clouds";
const char clouds3[] PROGMEM = "Some Clouds";
const char clouds4[] PROGMEM = "Mainly Cloudy";
const char clouds5[] PROGMEM = "Very Cloudy";
const char clouds6[] PROGMEM = "Overcast or Fog";

const char html_t2[] PROGMEM = "<div><canvas id=\"canvas\" height=\"250px\" width=\"800px\"></canvas>";
const char html_t4[] PROGMEM = "</div><center>Time in minutes</center></div><script>var lineChartData={labels:[";
const char html_t10[] PROGMEM = "],datasets:[{label:\"Sky delta\",fillColor:\"rgba(255,187,151,0.2)\",";
const char html_t14[] PROGMEM = "strokeColor:\"rgba(255,187,151,1)\",pointColor:\"rgba(255,187,151,1)\",";
const char html_t16[] PROGMEM = "pointStrokeColor:\"#fff\",pointHighlightFill:\"#fff\",";
const char html_t18[] PROGMEM = "pointHighlightStroke:\"rgba(255,187,151,1)\",data:[";

const char html_t20[] PROGMEM = "]},{label:\"Sky Temp\",fillColor:\"rgba(220,220,220,0.2)\",";
const char html_t24[] PROGMEM = "strokeColor:\"rgba(220,220,220,1)\",pointColor:\"rgba(220,220,220,1)\",";
const char html_t28[] PROGMEM = "pointHighlightStroke:\"rgba(220,220,220,1)\",data:[";

const char html_t30[] PROGMEM = "]},{label:\"Ambient Temp\",fillColor:\"rgba(151,187,255,0.2)\",";
const char html_t34[] PROGMEM = "strokeColor:\"rgba(151,187,255,1)\",pointColor:\"rgba(151,187,255,1)\",";
const char html_t38[] PROGMEM = "pointHighlightStroke:\"rgba(151,187,255,1)\",data:[";
const char html_t43[] PROGMEM= "]}]}\r\n window.onload=function(){var ctx=document.getElementById(\"canvas\").getContext(\"2d\");";
const char html_t45[] PROGMEM = "window.myLine=new Chart(ctx).Line(lineChartData,{responsive:true,animation:false,";
const char html_t46[] PROGMEM = "pointDotRadius:1,scaleOverride:true,scaleStartValue:-40,scaleStepWidth:10,scaleSteps:7});}</script>";

int read_pos = 0;
float gsa() // get ground (ambient) temp
{
    float f;

    if (read_pos<=0) read_pos+=1024;
    read_pos-=16;
    EEPROM_readQuad(read_pos,(byte*)&f); // sa

    return f;
}
float gss() // get sky temp
{
    float f;

    if (read_pos<=0) read_pos+=1024;
    read_pos-=12;
    EEPROM_readQuad(read_pos,(byte*)&f); // ss
    read_pos-=4;
    
    return f;
}
float gsad() // get average delta (cloud temp)
{
    float f;

    if (read_pos<=0) read_pos+=1024;
    read_pos-=8;
    EEPROM_readQuad(read_pos,(byte*)&f); // sad
    read_pos-=8;
    
    return f;
}

void index_html_page() {
  char temp[128] = "";
  char temp1[80] = "";
  char temp2[20] = "";
  char temp3[20] = "";
  int stp=0;
  html_page_step++;
    
  // send a standard http response header
  if (html_page_step==++stp) strcpy_P(temp, html_header12345); 
  if (html_page_step==++stp) strcpy_P(temp, html_header6);

  if (html_page_step==++stp) strcpy_P(temp, html_main_css123);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css4);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css5);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css6);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css7);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css8);
  
  if (html_page_step==++stp) strcpy_P(temp, html_header78);

  // and the remainder of the page
  if (html_page_step==++stp) strcpy_P(temp, html_index1);
  if (html_page_step==++stp) strcpy_P(temp, html_index2);
  if (html_page_step==++stp) strcpy_P(temp, html_index2a);
  if (html_page_step==++stp) strcpy_P(temp, html_links1);
  if (html_page_step==++stp) strcpy_P(temp, html_index2b);
  if (html_page_step==++stp) { 
    dtostrf(ambient_celsius,3,1,temp2);
    strcpy_P(temp1, html_index3); sprintf(temp,temp1,temp2);
  }
  if (html_page_step==++stp) { 
    dtostrf(sky_celsius,3,1,temp2);
    strcpy_P(temp1, html_index4); sprintf(temp,temp1,temp2,temp3);
  }
  if (html_page_step==++stp) {
    if (delta_celsius > SkyClear) strcpy_P(temp3,clouds1); else
    if (delta_celsius > SkyVSCldy) strcpy_P(temp3,clouds2); else
    if (delta_celsius > SkySCldy) strcpy_P(temp3,clouds3); else
    if (delta_celsius > SkyCldy) strcpy_P(temp3,clouds4); else
    if (delta_celsius > SkyVCldy) strcpy_P(temp3,clouds5); else strcpy_P(temp3,clouds6);

    dtostrf(delta_celsius,3,1,temp2);
    strcpy_P(temp1, html_index5); sprintf(temp,temp1,temp2,temp3);
  }
if (html_page_step==++stp) {
    if (avg_delta_celsius==invalid) strcpy(temp2,"Invalid"); else dtostrf(avg_delta_celsius,3,1,temp2);
    strcpy_P(temp1, html_index6); sprintf(temp,temp1,temp2); 
  }

  if (html_page_step==++stp) { 
    bool safe=true;
    // check for invalid rain sensor values
    if ((rainSensorReading2 < 0) || (avg_delta_celsius < -200)) safe=false;
    if ((rainSensorReading2 <= WetThreshold) || (avg_delta_celsius <= CloudThreshold)) safe=false;

    strcpy(temp,"<br><center><font class=\"");
    www_client.print(temp);

    if (safe) strcpy(temp,"g\">SAFE"); else strcpy(temp,"u\">UNSAFE");
    www_client.print(temp);

    strcpy(temp,"</font></center>");
  }

  if (html_page_step==++stp) {
//    1# is Rain, 2# is Warn, and 3# is Dry
    int r = rainSensorReading+1; 
    if (r==1) strcpy(temp2,"Rain");
    if (r==2) strcpy(temp2,"Warning");
    if (r==3) strcpy(temp2,"Dry");
    if (r<=0) strcpy(temp2,"Invalid");
    dtostrf(rainSensorReading2,1,3,temp3);
    strcpy_P(temp1, html_index7); sprintf(temp,temp1,temp3,temp2); 
  }
  if (html_page_step==++stp) {
#ifdef HEATER_ON
    sprintf(temp2,"Heater %i%%",heaterPower);
    strcpy_P(temp1, html_index8); sprintf(temp,temp1,temp2);
#else
    strcpy_P(temp1, html_index8); sprintf(temp,temp1,"");
#endif
  }
#ifdef HTU21D_ON
  if (html_page_step==++stp) {
    if (humiditySensorReading==invalid) strcpy(temp2,"Invalid"); else dtostrf(humiditySensorReading,4,2,temp2);
    strcpy_P(temp1, html_index9); sprintf(temp,temp1,temp2); 
  }
#endif
#ifdef BMP180_ON
  if (html_page_step==++stp) {
    if (pressureSensorReading==invalid) { strcpy(temp2,"Invalid"); strcpy(temp3,""); } else {
      dtostrf(pressureSensorReading,4,2,temp2);
      if (pressureDelta>0.5) strcpy(temp3," &uarr;"); else if (pressureDelta<-0.5) strcpy(temp3," &darr;"); else { strcpy(temp3," -"); }
    }
    strcpy_P(temp1, html_index10); sprintf(temp,temp1,temp2,temp3); 
  }
#endif

  if (html_page_step==++stp) strcpy(temp,"<br><br>");

#ifdef HTML_CHART_ON

  if (html_page_step==++stp) strcpy_P(temp, html_t2);  
  if (html_page_step==++stp) strcpy_P(temp, html_t4);  
  if (html_page_step==++stp) {
    // directly send the data from here to speed things up and to avoid buffer problems
    read_pos=log_pos;
    int j=0;
    for (int i=0; i<63; i++) {
      if (i%15==0) {
        if (j==0) sprintf(temp,"\"Now\","); else sprintf(temp,"\"T-%d\",",j);
        j=j+(SecondsBetweenLogEntries*15)/60;
      } else {
        sprintf(temp,"\"\",");
      }
      www_client.print(temp);
    }
    sprintf(temp,"\"\"");
  }
  if (html_page_step==++stp) strcpy_P(temp, html_t10);
  if (html_page_step==++stp) strcpy_P(temp, html_t14);  
  if (html_page_step==++stp) strcpy_P(temp, html_t16);  
  if (html_page_step==++stp) strcpy_P(temp, html_t18);  
  if (html_page_step==++stp) {
    // directly send the data from here to speed things up and to avoid buffer problems
    read_pos=log_pos;
    for (int i=0; i<log_count; i++) {
      dtostrf(gsa(),3,1,temp2);
      if (i<log_count-1) strcat(temp2,","); 
      www_client.print(temp2);
    }
  }
  if (html_page_step==++stp) strcpy_P(temp, html_t20);  
  if (html_page_step==++stp) strcpy_P(temp, html_t24);  
  if (html_page_step==++stp) strcpy_P(temp, html_t16);  
  if (html_page_step==++stp) strcpy_P(temp, html_t28);  
  if (html_page_step==++stp) {
    // directly send the data from here to speed things up and to avoid buffer problems
    read_pos=log_pos;
    for (int i=0; i<log_count; i++) {
      dtostrf(gsad(),3,1,temp2);
      if (i<log_count-1) strcat(temp2,","); 
      www_client.print(temp2);
    }
  }
  if (html_page_step==++stp) strcpy_P(temp, html_t30);  
  if (html_page_step==++stp) strcpy_P(temp, html_t34);  
  if (html_page_step==++stp) strcpy_P(temp, html_t16);  
  if (html_page_step==++stp) strcpy_P(temp, html_t38);  
  if (html_page_step==++stp) {
    // directly send the data from here to speed things up and to avoid buffer problems
    read_pos=log_pos;
    for (int i=0; i<log_count; i++) {
      dtostrf(gss(),3,1,temp2);
      if (i<log_count-1) strcat(temp2,","); 
      www_client.print(temp2);
    }
  }
  if (html_page_step==++stp) strcpy_P(temp, html_t43);  
  if (html_page_step==++stp) strcpy_P(temp, html_t45);  
  if (html_page_step==++stp) strcpy_P(temp, html_t46);  
#endif
  
  if (html_page_step==++stp) strcpy(temp,"</body></html>");

  // stop sending this page
  if (html_page_step==++stp) { html_page_step=0; responseStarted=false; clientNeedsToClose=true; responseFinish_ms=millis(); return; }

  // send the data
  www_client.print(temp);
}

#ifdef SD_CARD_ON
const char html_header1j[] PROGMEM = "HTTP/1.1 200 OK\r\n";
//const char html_header2j[] PROGMEM = "Date: Thu, 25 Feb 2016 17:00:49 GMT\r\n";
const char html_header3j[] PROGMEM = "Etag: \"3457807a63ac7bdabf8999b98245d0fe\"\r\n";
const char html_header4j[] PROGMEM = "Last-Modified: Mon, 13 Apr 2015 15:35:56 GMT\r\n";
const char html_header5j[] PROGMEM = "Content-Type: Content-Type: application/javascript\r\n";
const char html_header6j[] PROGMEM = "Connection: close\r\n\r\n";

const char html_header1k[] PROGMEM = "HTTP/1.1 304 OK\r\n";
const char html_header4k[] PROGMEM = "Last-Modified: Mon, 13 Apr 2015 15:35:56 GMT\r\n";
const char html_header6k[] PROGMEM = "Connection: close\r\n\r\n";
// Send the "Chart.min.js" javascript from the sdcard ----------------------------------------------------------------------------
// 
File dataFile;
bool fileOpen = false;
void chart_js_html_page() {
  char temp[256] = "";

  if (!fileOpen) {
    // open the sdcard file
    if (sdReady) {

      dataFile = SD.open("Chart.js", FILE_READ);
      if (dataFile) fileOpen=true; else fileOpen=false;
    } else exit;

    if (modifiedSince_found) {
      // send the 304 header
      strcpy_P(temp, html_header1k); www_client.print(temp);
      strcpy_P(temp, html_header4k); www_client.print(temp);
      strcpy_P(temp, html_header6k); www_client.print(temp);
      dataFile.close(); fileOpen=false;
      modifiedSince_found=false;
    } else {
      // send the header
      strcpy_P(temp, html_header1j); www_client.print(temp);
//    strcpy_P(temp, html_header2j); www_client.print(temp);
      strcpy_P(temp, html_header3j); www_client.print(temp);
      strcpy_P(temp, html_header4j); www_client.print(temp);
      strcpy_P(temp, html_header5j); www_client.print(temp);
      strcpy_P(temp, html_header6j); www_client.print(temp);
    }
  } else {
     int n = dataFile.available();
     if (n > 256) n = 256;
     dataFile.read(temp, n);
     www_client.write(temp, n);
                    
    // end of file?
    if (n==0) { dataFile.close(); fileOpen=false; }
  }

  // stop sending this page
  if (!fileOpen) { responseStarted=false; clientNeedsToClose=true; responseFinish_ms=millis(); return; }
}
#endif

#endif

