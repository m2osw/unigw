// TCP Client & Server -- classes to ease handling sockets
// Copyright (C) 2012-2013  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#ifndef SNAP_TCP_CLIENT_SERVER_H
#define SNAP_TCP_CLIENT_SERVER_H

/** \file
 * \brief Declaration of the TCP client/server classes.
 *
 * This header includes the TCP client and TCP server classes used to create
 * a server and a client to access the server.
 *
 * wpkg primarily uses the tcp_client class to access websites and other
 * remote data.
 */

#if defined(MO_WINDOWS)
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif
#include <stdexcept>
#if defined(MO_FREEBSD)
#include <netinet/in.h>
#endif


namespace tcp_client_server
{

#if defined(MO_WINDOWS)
typedef SOCKET socket_t;
#else
typedef int socket_t;
#define INVALID_SOCKET (-1)
#endif

void initialize_winsock();

class tcp_client_server_logic_error : public std::logic_error
{
public:
    tcp_client_server_logic_error(const std::string& errmsg) : logic_error(errmsg) {}
};

class tcp_client_server_runtime_error : public std::runtime_error
{
public:
    tcp_client_server_runtime_error(const std::string& errmsg) : runtime_error(errmsg) {}
};

class tcp_client_server_parameter_error : public tcp_client_server_logic_error
{
public:
    tcp_client_server_parameter_error(const std::string& errmsg) : tcp_client_server_logic_error(errmsg) {}
};



class tcp_client
{
public:
                        tcp_client(const std::string& addr, int port);
                        ~tcp_client();

    socket_t            get_socket() const;
    int                 get_port() const;
    std::string         get_addr() const;

    int                 read(char *buf, size_t size);
    int                 write(const char *buf, size_t size);

private:
    socket_t            f_socket;
    int                 f_port;
    std::string         f_addr;
};


class tcp_server
{
public:
    static const int    MAX_CONNECTIONS = 50;

                        tcp_server(const std::string& addr, int port, int max_connections = -1, bool reuse_addr = false, bool auto_close = false);
                        ~tcp_server();

    socket_t            get_socket() const;
    int                 get_max_connections() const;
    int                 get_port() const;
    std::string         get_addr() const;
    bool                get_keepalive() const;

    void                keepalive(bool yes = true);
    socket_t            accept();
    socket_t            get_last_accepted_socket() const;

private:
    int                 f_max_connections;
    socket_t            f_socket;
    int                 f_port;
    std::string         f_addr;
    socket_t            f_accepted_socket;
    struct sockaddr_in  f_accepted_addr;
    bool                f_keepalive;
    bool                f_auto_close;
};


} // namespace tcp_client_server
#endif
// SNAP_TCP_CLIENT_SERVER_H
// vim: ts=4 sw=4 et
