#ifndef SERVER_H
#define SERVER_H

#include "server_global.h"

#include<iostream>
#include<fstream>

using namespace std;

class SERVERSHARED_EXPORT Server
{

public:
    Server();
    ~Server();

private:
    inline void Call_Andor_API(unsigned int err_code, const char *file, int line,
                               ostream &log_file = cerr);

};

/* Andor API wrapper macro definition */
#define API_CALL(err_code) { Server::Call_Andor_API((err_code), __FILE__, __LINE__); }

#endif // SERVER_H
