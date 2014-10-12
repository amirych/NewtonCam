#include "server.h"


Server::Server()
{
    API_CALL(10);
    API_CALL(101);
}

Server::~Server()
{
}

/*  Private members  */

void Server::Call_Andor_API(unsigned int err_code, const char *file, int line, ostream &log_file)
{
    log_file << "ANDOR API CALL: error = " << err_code << " (file: " << file <<
                ", line: " << line << std::endl << std::flush;
}
