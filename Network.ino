#if defined(W5100_ON)

// The Ethernet channel code below was written by Luka
// you can telnet into it at port 9999
// provides the same functions as for serial

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:

// for ethernet
int  www_xmit_buffer_send_pos=0;
int  www_xmit_buffer_pos=0;
char www_xmit_buffer[256] = "";

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 1, 56);
IPAddress myDns(192,168,1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);


// Network server port for command channel
EthernetServer cmd_server(9999);
EthernetClient cmd_client;
// Network server port for web channel
EthernetServer web_server(80);
EthernetClient www_client;
boolean alreadyConnected = false; // whether or not the client was connected previously


void Ethernet_Init() {
  // initialize the ethernet device
#if defined(ETHERNET_USE_DHCP_ON)
  Ethernet.begin(mac);
#else
  Ethernet.begin(mac, ip, myDns, gateway, subnet);
#endif

  cmd_server.begin();
  web_server.begin();
  www_client = web_server.available(); // initialise www_client
}

void Ethernet_send(const char data[]) {
  if (!cmd_client)
   return;
   
  cmd_client.flush();
   
  cmd_client.print(data);
//  do {} while (Serial_transmit());
}

void Ethernet_print(const char data[]) {
  if (!cmd_client)
   return;
   
   cmd_server.print(data);
 }
 
 boolean Ethernet_transmit() {
   return false;
 }
 
 boolean Ethernet_available() {
   cmd_client = cmd_server.available();
   return cmd_client;
 }
 
 char Ethernet_read() {
   if (!cmd_client)
   return -1;
   
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

bool get_check=false;
bool get_name=false;
bool get_val=false;
int get_idx=0;
char get_names[11] = "";
char get_vals[11] = "";

// variables to support web-page request detection
const char index_page[] = "GET /index.htm"; bool index_page_found; byte index_page_count;
/* add additional web-pages per the above line(s) here */

void reset_page_requests() {
  index_page_found=false; index_page_count=0;
/* add additional web-pages per the above line(s) here */

  get_check=false; get_val=false; get_name=false;
}

void Ethernet_www() {
  // if a client doesn't already exist try to find a new one
  if (!www_client) { www_client = web_server.available(); currentLineIsBlank = true; responseStarted=false; clientNeedsToClose=false; reset_page_requests(); transactionStart_ms=millis(); }

  // active client?
  if (www_client) {
    if (www_client.connected()) {
      if (www_client.available()) {
        char c = www_client.read();

        //Serial_char(c);

        // record the get request name and value, then process the request
        if ((get_val) && ((c==' ') || (c=='&'))) { get_val=false; get_idx=0; Ethernet_get(); if (c=='&') { c='?'; get_check=true; } }
        if (get_val) { if (get_idx<10) { get_vals[get_idx]=c; get_vals[get_idx+1]=0; get_idx++; } }
        if ((get_name) && (c=='=')) { get_name=false; get_val=true; get_idx=0; }
        if (get_name) { if (get_idx<10) { get_names[get_idx]=c; get_names[get_idx+1]=0; get_idx++; } }
        if ((get_check) && (c=='?')) { get_name=true; get_idx=0; } get_check=false;
        
        // watch for page requests and flag them
        if (!index_page_found)    { if (c==index_page[index_page_count])       index_page_count++;    else index_page_count=0;    if (index_page_count==14)    { index_page_found=true; get_check=true; } }
/* add additional web-pages per the above line(s) here */
        
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
/* add additional web-pages per the above line(s) here */

        index_html_page();
      }

      if (www_xmit_buffer_pos>0) {
        if (!www_send()) {
          clientNeedsToClose=true;
          responseFinish_ms=millis();
        } else {
          clientNeedsToClose=false;
        }
      }

    }
    // if data was sent give the web browser time to receive it then stop the client
    // if a transaction is taking more than five seconds, stop the client
    if ((clientNeedsToClose && (millis()-responseFinish_ms>100)) || (millis()-transactionStart_ms>5000)) {
      clientNeedsToClose=false;
      www_client.stop();
    }
    Ethernet.maintain();
  }
}

// quickly writes up to 5 chars at a time from buffer to ethernet adapter
// returns true if data is still waiting for transmit
// returns false if data buffer is empty
boolean www_send() {
  char buf[11] = "";
  char c;
  
  // copy some data
  boolean buffer_empty=false;
  for (int l=0; l<5; l++) {
    c=www_xmit_buffer[www_xmit_buffer_send_pos];
    buf[l+1]=0;
    buf[l]=c;
    if ((c==0) || (www_xmit_buffer_send_pos>254)) { buffer_empty=true; break; }
    www_xmit_buffer_send_pos++;
  }

  // send network data
  www_client.print(buf);

  // hit end of www_xmit_buffer? reset and start over
  if (buffer_empty) {
    www_xmit_buffer_pos=0;
    www_xmit_buffer_send_pos=0;
    www_xmit_buffer[0]=0;
    return false;
  }

  return true;
}

// quickly copies string to www server transmit buffer
// returns true if successful
// returns false if buffer is too full to accept the data (and copies no data into buffer)
boolean www_write(const char data[]) {
  int l=strlen(data);
  if (www_xmit_buffer_pos+l>254) return false;
  strcpy((char *)&www_xmit_buffer[www_xmit_buffer_pos],data);
  www_xmit_buffer_pos+=l;
  return true;
}

// ---------------------------------------------------------------------------------------------------------
// process GET requests 
// currently only the name and value are handled, the page sending the name isn't considered
// this lowers the processing overhead and is sufficient for our purposes here
double get_temp_float;
int get_temp_day,get_temp_month,get_temp_year;
int get_temp_hour,get_temp_minute,get_temp_second;
void Ethernet_get() {
  if (get_names[2]!=0) return; // only two char names for now

  // from the Index.htm page -------------------------------------------------------------------
  // Example, sends s1=f to us here: "index.htm?s1=f"
  if ((get_names[0]=='s') && (get_names[1]=='1')) {
    if ((get_vals[0]=='f') && (get_vals[1]==0)) {
      // do something here
    };
  }
}

// --------------------------------------------------------------------------------------------------------------
// Web-site pages, the design of each xxxxx_html_page() function is now capable of sending a web-page of any size
// since it stops and waits should the local outgoing buffer become full
const char html_header1[] PROGMEM = "HTTP/1.1 200 OK\r\n";
const char html_header2[] PROGMEM = "Content-Type: text/html\r\n";
const char html_header3[] PROGMEM = "Connection: close\r\n";
const char html_header4[] PROGMEM = "Refresh: 5\r\n\r\n";
const char html_header4a[] PROGMEM = "\r\n";
const char html_header5[] PROGMEM = "<!DOCTYPE HTML>\r\n<html>\r\n";
const char html_header6[] PROGMEM = "<head>\r\n";
const char html_header7[] PROGMEM = "</head>\r\n";
const char html_header8[] PROGMEM = "<body bgcolor=\"#26262A\">\r\n";

const char html_main_css1[] PROGMEM = "<STYLE>\r\n";
const char html_main_css2[] PROGMEM = ".a { background-color: #111111; }\r\n";
const char html_main_css3[] PROGMEM = ".t { padding: 15px; border: 15px solid #551111;\r\n";
const char html_main_css4[] PROGMEM = " margin: 25px; color: #999999; background-color: #111111; }\r\n";
const char html_main_css5[] PROGMEM = ".b { padding: 30px; border: 2px solid #551111;\r\n";
const char html_main_css6[] PROGMEM = " margin: 30px; color: #999999; background-color: #111111; }\r\n";
const char html_main_css7[] PROGMEM = "h1 { text-align: right; }\r\n";
const char html_main_css8[] PROGMEM = "input { width:4em }\r\n";
const char html_main_css9[] PROGMEM = "</STYLE>\r\n";

const char html_links1[] PROGMEM = "<a href=\"/index.htm\">Status</a>";
const char html_links2[] PROGMEM = "</a><br />";

// The index.htm page --------------------------------------------------------------------------------------
const char html_index1[] PROGMEM = "<div class=\"t\"><table width=\"100%\"><tr><td><b>" FirmwareName " " FirmwareNumber;
const char html_index2[] PROGMEM = "</b></td><td align=\"right\"><b><font size=\"5\">";
const char html_index2a[] PROGMEM = "STATUS</font></b></td></tr></table><br />";
const char html_index2b[] PROGMEM = "</div><div class=\"b\"><br />";
const char html_index3[] PROGMEM = "DS18B20 Temperature %s<br />";
const char html_index4[] PROGMEM = "MLX90614 Temperature %s<br /><br />";
const char html_index5[] PROGMEM = "Delta Temperature %s<br />";
const char html_index6[] PROGMEM = "Averaged delta Temperature %s<br /><br />";
const char html_index7[] PROGMEM = "Rain sensor status: %s<br />";
const char html_index8[] PROGMEM = "(Rain sensor reading as float %s)<br />";


void index_html_page() {
  char temp[128] = "";
  char temp1[80] = "";
  char temp2[20] = "";
  char temp3[20] = "";
  bool r=true;
  int stp=0;
  html_page_step++;
    
  // send a standard http response header
  if (html_page_step==++stp) strcpy_P(temp, html_header1); 
  if (html_page_step==++stp) strcpy_P(temp, html_header2);
  if (html_page_step==++stp) strcpy_P(temp, html_header3);
  if (html_page_step==++stp) strcpy_P(temp, html_header4);
  if (html_page_step==++stp) strcpy_P(temp, html_header5);
  if (html_page_step==++stp) strcpy_P(temp, html_header6);

  if (html_page_step==++stp) strcpy_P(temp, html_main_css1);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css2);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css3);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css4);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css5);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css6);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css7);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css8);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css9);
  
  if (html_page_step==++stp) strcpy_P(temp, html_header7);
  if (html_page_step==++stp) strcpy_P(temp, html_header8);

  // and the remainder of the page
  if (html_page_step==++stp) strcpy_P(temp, html_index1);
  if (html_page_step==++stp) strcpy_P(temp, html_index2);
  if (html_page_step==++stp) strcpy_P(temp, html_index2a);
  if (html_page_step==++stp) strcpy_P(temp, html_links1);
  if (html_page_step==++stp) strcpy_P(temp, html_links2);
  if (html_page_step==++stp) strcpy_P(temp, html_index2b);
  if (html_page_step==++stp) { 
    dtostrf(ds18b20_celsius,3,1,temp2);
    strcpy_P(temp1, html_index3); sprintf(temp,temp1,temp2);
  }
  if (html_page_step==++stp) { 
    dtostrf(MLX90614_celsius,3,1,temp2);
    strcpy_P(temp1, html_index4); sprintf(temp,temp1,temp2);
  }
  if (html_page_step==++stp) { 
    dtostrf(delta_celsius,3,1,temp2);
    strcpy_P(temp1, html_index5); sprintf(temp,temp1,temp2);
  }
  if (html_page_step==++stp) {
    dtostrf(avg_delta_celsius,3,1,temp2);
    strcpy_P(temp1, html_index6); sprintf(temp,temp1,temp2); 
  }
  if (html_page_step==++stp) {
//    1# is Rain, 2# is Warn, and 3# is Dry
    int r = rainSensorReading+1; 
    if (r<=0) r=invalid;
    if (r==1) strcpy(temp2,"Raining");
    if (r==2) strcpy(temp2,"Warning");
    if (r==3) strcpy(temp2,"Dry");
    if (r<=0) strcpy(temp2,"Invalid");
//    sprintf(temp2,"%d",r);
    strcpy_P(temp1, html_index7); sprintf(temp,temp1,temp2); 
  }
  if (html_page_step==++stp) {
   dtostrf(rainSensorReading2,1,3,temp2);
   strcpy_P(temp1, html_index8); sprintf(temp,temp1,temp2);
  }
  if (html_page_step==++stp) strcpy(temp,"</div></body></html>");

  // stop sending this page
  if (html_page_step==++stp) { html_page_step=0; responseStarted=false; return; }

  // send the data
  r=www_write(temp);
  if (!r) html_page_step--; // repeat this step if www_write failed
}

#endif
