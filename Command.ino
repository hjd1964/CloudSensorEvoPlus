// Command processing

boolean serial_zero_ready=false;
#if defined(W5100_ON)
boolean ethernet_ready = false;
#endif
boolean commandError;
boolean quietReply;
char command[3];
char parameter[17];
char reply[32];

byte bufferPtr_serial_zero=0;
char command_serial_zero[25];
char parameter_serial_zero[25];

byte bufferPtr_ethernet= 0;
char command_ethernet[25];
char parameter_ethernet[25];

enum Command {COMMAND_NONE, COMMAND_SERIAL, COMMAND_ETHERNET};

// process commands
void processCommands() {
    boolean supress_frame = false;
    char *conv_end;
    long i=0;

    if ((Serial.available() > 0) && (!serial_zero_ready)) { serial_zero_ready = buildCommand(Serial.read()); }
#if defined(W5100_ON)
    if ((Ethernet_available() > 0) && (!ethernet_ready)) { ethernet_ready = buildCommand_ethernet(Ethernet_read()); }
    if (Ethernet_transmit()) return;
#endif

    byte process_command = COMMAND_NONE;
    if (serial_zero_ready)     { strcpy(command,command_serial_zero); strcpy(parameter,parameter_serial_zero); serial_zero_ready=false; clearCommand_serial_zero(); process_command=COMMAND_SERIAL; }
#if defined(W5100_ON)
    else if (ethernet_ready)   { strcpy(command,command_ethernet);    strcpy(parameter,parameter_ethernet);    ethernet_ready=false;    clearCommand_ethernet();    process_command=COMMAND_ETHERNET; }
#endif
    else {
#if defined(W5100_ON)
      Ethernet_www();
#endif
      return;
    }

    if (process_command) {

          

// Command is two chars followed by an optional parameter...
      commandError=false;
      quietReply=false;
// Handles empty and one char replies
      reply[0]=0; reply[1]=0;

//   I - Info Commands
      if (command[0]=='I') {
//  :IN#  get version Number
//         Returns: s#
        if (command[1]=='N') {
          strcpy(reply,FirmwareNumber);
          quietReply=true;
        } else
//  :IP#  get Product
//         Returns: s#
        if (command[1]=='P') {
          strcpy(reply,FirmwareName);
          quietReply=true;
        } else commandError=true;
      } else

//   G - Get Commands
      if (command[0]=='G') {
//  :G1#  Get DS18B20 Temperature
//         Returns: nnn.n#
        if (command[1]=='1') {
          dtostrf(ambient_celsius,3,1,reply);
          quietReply=true;
        } else
//  :G2#  Get MLX90614 Temperature
//         Returns: nnn.n#
        if (command[1]=='2') {
          dtostrf(MLX90614_celsius,3,1,reply);
          quietReply=true;
        } else
//  :G3#  Get delta Temperature
//         Returns: nnn.n#
//         where <=21 is cloudy
        if (command[1]=='3') {
          //delta_celsius=22.0;
          dtostrf(delta_celsius,3,1,reply);
          quietReply=true;
        } else
//  :GS#  Get averaged delta Temperature
//         Returns: nnn.n#
//         where <=21 is cloudy
        if (command[1]=='S') {
          dtostrf(avg_delta_celsius,3,1,reply);
          quietReply=true;
        } else
//  :GR#  Get rain sensor status
//         Returns: n# 
//         where 1# is Rain, 2# is Warn, and 3# is Dry
        if (command[1]=='R') {
          int r = rainSensorReading+1; 
          if (r<=0) r=invalid;
          sprintf(reply,"%d",r);
          quietReply=true;
        } else
//  :Gr#  Get rain sensor reading as Float
//         Returns: n.nnn#
//         where n ranges from 0.0 to 1.0
        if (command[1]=='r') {
          dtostrf(rainSensorReading2,1,3,reply);
          quietReply=true;
        } else
//  :Gh#  Get relative humidity reading as Float
//         Returns: n.n#
//         where n ranges from 0.0 to 100.0
        if (command[1]=='h') {
          dtostrf(humiditySensorReading,1,1,reply);
          quietReply=true;
        } else
//  :Gb#  Get barometric reading as Float
//         Returns: n.nnn#
//         where n ranges from about 980.0 to 1050.0 (mbar, sea-level compensated)
        if (command[1]=='b') {
          dtostrf(pressureSensorReading,1,1,reply);
          quietReply=true;
        } else commandError=true;
      } else commandError=true;
    }

    if (!quietReply) {
      if (commandError) reply[0]='0'; else reply[0]='1';
      reply[1]=0;
      supress_frame=true;
    }
    
    if (strlen(reply)>0) {

    if (process_command==COMMAND_SERIAL) {
#ifdef CHKSUM0_ON
      // calculate the checksum
      char HEXS[3]="";
      byte cks=0; for (int cksCount0=0; cksCount0<strlen(reply); cksCount0++) {  cks+=reply[cksCount0]; }
      sprintf(HEXS,"%02X",cks);
      strcat(reply,HEXS);
#endif
      strcat(reply,"#");
      Serial.print(reply);
    }

#if defined(W5100_ON)
      if (process_command==COMMAND_ETHERNET) {
#ifdef CHKSUM0_ON
        // calculate the checksum
        char HEXS[3]="";
        byte cks=0; for (int cksCount0=0; cksCount0<strlen(reply); cksCount0++) {  cks+=reply[cksCount0]; }
        sprintf(HEXS,"%02X",cks);
        strcat(reply,HEXS);
#endif
        if (!supress_frame) strcat(reply,"#");
        Ethernet_print(reply);
      }
#endif       

    }
    quietReply=false;
    
  }

// Build up a command
boolean buildCommand(char c) {
  // ignore spaces/lf/cr, dropping spaces is another tweek to allow compatibility with LX200 protocol
  if ((c!=(char)32) && (c!=(char)10) && (c!=(char)13) && (c!=(char)6)) {
    command_serial_zero[bufferPtr_serial_zero]=c;
    bufferPtr_serial_zero++;
    command_serial_zero[bufferPtr_serial_zero]=(char)0;
    if (bufferPtr_serial_zero>22) { bufferPtr_serial_zero=22; }  // limit maximum command length to avoid overflow, c2+p16+cc2+eol2+eos1=23 bytes max ranging from 0..22
  }
  
  if (c=='#') {
    // validate the command frame, normal command
    if ((bufferPtr_serial_zero>1) && (command_serial_zero[0]==':') && (command_serial_zero[bufferPtr_serial_zero-1]=='#')) { command_serial_zero[bufferPtr_serial_zero-1]=0; } else { clearCommand_serial_zero(); return false; }

#ifdef CHKSUM0_ON
    // checksum the data, for example ":11111126".  I don't include the command frame in the checksum.  The error response is a checksumed null string "00#" which means re-transmit.
    byte len=strlen(command_serial_zero);
    byte cks=0; for (int cksCount0=1; cksCount0<len-2; cksCount0++) {  cks+=command_serial_zero[cksCount0]; }
    char chkSum[3]; sprintf(chkSum,"%02X",cks); if (!((chkSum[0]==command_serial_zero[len-2]) && (chkSum[1]==command_serial_zero[len-1]))) { clearCommand_serial_zero();  Serial.print("00#"); return false; }
    --len; command_serial_zero[--len]=0;
#endif

    // break up the command into a two char command and the remaining parameter
    
    // the parameter can be up to 16 chars in length
    memmove(parameter_serial_zero,(char *)&command_serial_zero[3],17);

    // the command is either one or two chars in length
    command_serial_zero[3]=0;  memmove(command_serial_zero,(char *)&command_serial_zero[1],3);

    return true;
  } else {
    return false;
  }
}

// clear commands
boolean clearCommand_serial_zero() {
  bufferPtr_serial_zero=0;
  command_serial_zero[bufferPtr_serial_zero]=(char)0;
  return true;
}

#if defined(W5100_ON)
// Build up a command
boolean buildCommand_ethernet(char c) {
  // return if -1 is received (no data)
  if (c == 0xFF)
    return false;

  // (chr)6 is a special status command for the LX200 protocol
  if ((c==(char)6) && (bufferPtr_ethernet==0)) {
//    Ethernet_print("G#");
    #ifdef MOUNT_TYPE_ALTAZM
    Ethernet_print("A");
    #else
    Ethernet_print("P");
    #endif
  }

  // ignore spaces/lf/cr, dropping spaces is another tweek to allow compatibility with LX200 protocol
  if ((c!=(char)32) && (c!=(char)10) && (c!=(char)13) && (c!=(char)6)) {
    command_ethernet[bufferPtr_ethernet]=c;
    bufferPtr_ethernet++;
    command_ethernet[bufferPtr_ethernet]=(char)0;
    if (bufferPtr_ethernet>22) { bufferPtr_ethernet=22; }  // limit maximum command length to avoid overflow, c2+p16+cc2+eol2+eos1=23 bytes max ranging from 0..22
  }

  if (c=='#') {
    // validate the command frame, normal command
    if ((bufferPtr_ethernet>1) && (command_ethernet[0]==':') && (command_ethernet[bufferPtr_ethernet-1]=='#')) { command_ethernet[bufferPtr_ethernet-1]=0; } else { clearCommand_ethernet(); return false; }

#ifdef CHKSUM1_ON
    // checksum the data, as above.
    byte len=strlen(command_ethernet);
    byte cks=0; for (int cksCount0=1; cksCount0<len-2; cksCount0++) { cks=cks+command_ethernet[cksCount0]; }
    char chkSum[3]; sprintf(chkSum,"%02X",cks); if (!((chkSum[0]==command_ethernet[len-2]) && (chkSum[1]==command_ethernet[len-1]))) { clearCommand_ethernet(); Ethernet_print("00#"); return false; }
    --len; command_ethernet[--len]=0;
#endif

    // break up the command into a two char command and the remaining parameter

    // the parameter can be up to 16 chars in length
    memmove(parameter_ethernet,(char *)&command_ethernet[3],17);

    // the command is either one or two chars in length
    command_ethernet[3]=0;  memmove(command_ethernet,(char *)&command_ethernet[1],3);

    return true;
  } else {
    return false;
  }
}

// clear commands
boolean clearCommand_ethernet() {
  bufferPtr_ethernet=0;
  command_ethernet[bufferPtr_ethernet]=(char)0;
  return true;
}
#endif


