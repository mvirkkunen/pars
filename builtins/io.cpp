#include <cstdio>
#include <cstring>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "builtins.hpp"

namespace pars { namespace builtins {

BUILTIN("print") print(Context &c, Value rest) {
    for (; is_cons(rest); rest = cdr(rest)) {
        if (type_of(car(rest)) == Type::str) {
            printf("%.*s", str_len(car(rest)), str_data(car(rest)));
        } else {
            c.print(car(rest), false);
            printf(" ");
        }
    }

    printf("\n");

    return nil;
}

void destroy_socket(void *ptr) {
    int fd = (int)(uintptr_t)ptr;
    if (fd)
        close(fd);
}

Type type_socket = register_type("socket", nullptr, destroy_socket);

inline int fd_of(Value sock) { return (int)(uintptr_t)ptr_of(sock); }

#define VERIFY_ARG_SOCKET(ARG, N) \
    if (type_of(ARG) != type_socket) return c.error("Argument %d must be a socket.", N); \
    if (!ptr_of(ARG)) return c.error("Socket is closed.")

BUILTIN("socket-connect") socket_connect(Context &c, Value address_, Value port_) {
    VERIFY_ARG_STR(address_, 1);
    VERIFY_ARG_NUM(port_, 2);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));

    hints.ai_socktype = SOCK_STREAM;

    char port[20];
    snprintf(port, sizeof(port), "%d", num_val(port_));

    struct addrinfo *res;
    if (getaddrinfo(str_data(address_), port, &hints, &res))
        return c.error("getaddrinfo() error");

    if (res == nullptr)
        return c.error("Host not found");

    for (struct addrinfo *rp = res; rp; rp = rp->ai_next) {
        int fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd < 0)
            continue;

        if (!connect(fd, rp->ai_addr, rp->ai_addrlen))
            return c.ptr(type_socket, (void *)(uintptr_t)fd);

        close(fd);
    }

    return c.error("connect() error");
}

BUILTIN("socket-send") socket_send(Context &c, Value sock, Value data) {
    VERIFY_ARG_SOCKET(sock, 1);
    VERIFY_ARG_STR(data, 2);

    int res = send(fd_of(sock), str_data(data), str_len(data), 0);
    if (res < 0)
        return c.error("send() error");

    return c.num(res);
}

BUILTIN("socket-recv") socket_recv(Context &c, Value sock, Value max_len_) {
    VERIFY_ARG_SOCKET(sock, 1);
    VERIFY_ARG_NUM(max_len_, 2);

    String *str = string_alloc(num_val(max_len_));

    int res = recv(fd_of(sock), str->data, str->len, 0);
    if (res < 0)
        return c.error("recv() error");

    if (res == 0) {
        free(str);
        return c.str_empty();
    }

    string_realloc(&str, res);

    return c.str(str);
}

BUILTIN("socket-close") socket_close(Context &c, Value sock) {
    VERIFY_ARG_SOCKET(sock, 1);

    close(fd_of(sock));
    set_ptr_of(sock, nullptr);

    return nil;
}

} }
