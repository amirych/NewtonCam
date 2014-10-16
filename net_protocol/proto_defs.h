#ifndef PROTO_DEFS_H
#define PROTO_DEFS_H


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

#endif // PROTO_DEFS_H
