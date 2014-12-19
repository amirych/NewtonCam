#ifndef PROTO_DEFS_H
#define PROTO_DEFS_H

#define NETPROTOCOL_DEFAULT_PORT 7777      // default network port
#define NETPROTOCOL_TIMEOUT 30000          // default timeout for network read-write (im millisecs)
#define NETPROTOCOL_CONTENT_DELIMETER " "  // default delimeter in content (e.g. between command name and its arguments)

#define NETPROTOCOL_ID_FIELD_LEN 1
#define NETPROTOCOL_SIZE_FIELD_LEN 4

#define NETPROTOCOL_MAX_CONTENT_LEN 9999

//#define NETPROTOCOL_PACKET_ID_INFO   0
//#define NETPROTOCOL_PACKET_ID_CMD    1
//#define NETPROTOCOL_PACKET_ID_TEMP   3
//#define NETPROTOCOL_PACKET_ID_STATUS 4
//#define NETPROTOCOL_PACKET_ID_HELLO  5


#define NETPROTOCOL_ERROR_UNKNOWN -999999

#define NETPROTOCOL_ERROR_OK               1000
#define NETPROTOCOL_ERROR_UNKNOWN_PROTOCOL 1001
#define NETPROTOCOL_ERROR_CONTENT_LEN 1002       // content length is greater than permitted


            /*  Sender types  */

#define NETPROTOCOL_SENDER_TYPE_SERVER "NEWTON-SERVER"
#define NETPROTOCOL_SENDER_TYPE_CLIENT "NEWTON-CLIENT"
#define NETPROTOCOL_SENDER_TYPE_GUI    "NEWTON-GUI"

            /*  Protocol commands    */

#define NETPROTOCOL_COMMAND_INIT     "INIT"     // initialize camera
#define NETPROTOCOL_COMMAND_STOP     "STOP"     // stop aquisition process
#define NETPROTOCOL_COMMAND_GAIN     "GAIN"     // set gain
#define NETPROTOCOL_COMMAND_BINNING  "BIN"      // set binning
#define NETPROTOCOL_COMMAND_EXPTIME  "EXPTIME"  // exposure time
#define NETPROTOCOL_COMMAND_RATE     "RATE"     // set read-out rate
#define NETPROTOCOL_COMMAND_ROI      "ROI"      // set read-out region
#define NETPROTOCOL_COMMAND_SHUTTER  "SHUTTER"  // set shutter state
#define NETPROTOCOL_COMMAND_COOLER   "COOLER"   // set cooler state
#define NETPROTOCOL_COMMAND_FAN       "FAN"     // set fan state
#define NETPROTOCOL_COMMAND_SETTEMP  "SETTEMP"  // set CCD chip temperature
#define NETPROTOCOL_COMMAND_GETTEMP  "GETTEMP"  // get CCD chip temperature
#define NETPROTOCOL_COMMAND_FITSFILE "FITSFILE" // name of output FITS file
#define NETPROTOCOL_COMMAND_HEADFILE "HEADFILE" // name of FITS header file

#endif // PROTO_DEFS_H
