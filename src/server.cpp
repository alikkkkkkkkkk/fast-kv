#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

#include "kv_store.h"
#include "thread_pool.h"
#include "logger.h"
#include "stats.h"

int make_socket_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) return -1;
    return 0;
}

int main() {
    log_init("fast-kv.log");
    log_message("[start] fast-kv epoll/thread-pool server");

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        log_message("[fatal] cannot create socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        log_message("[fatal] bind failed");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 128) < 0) {
        log_message("[fatal] listen failed");
        close(server_fd);
        return 1;
    }

    if (make_socket_non_blocking(server_fd) < 0) {
        log_message("[fatal] cannot set server socket non-blocking");
        close(server_fd);
        return 1;
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        log_message("[fatal] epoll_create1 failed");
        close(server_fd);
        return 1;
    }

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) < 0) {
        log_message("[fatal] epoll_ctl ADD server_fd failed");
        close(epoll_fd);
        close(server_fd);
        return 1;
    }

    ThreadPool pool(std::thread::hardware_concurrency() == 0 ? 4 : std::thread::hardware_concurrency());

    std::unordered_map<int, std::string> buffers;

    std::vector<epoll_event> events(64);

    while (true) {
        int n = epoll_wait(epoll_fd, events.data(), static_cast<int>(events.size()), -1);
        if (n < 0) {
            if (errno == EINTR) continue;
            log_message("[fatal] epoll_wait failed");
            break;
        }

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            uint32_t evs = events[i].events;

            if (fd == server_fd) {
                while (true) {
                    int client_fd = accept(server_fd, nullptr, nullptr);
                    if (client_fd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        log_message("[error] accept failed");
                        break;
                    }

                    if (make_socket_non_blocking(client_fd) < 0) {
                        log_message("[error] cannot set client socket non-blocking");
                        close(client_fd);
                        continue;
                    }

                    epoll_event client_ev{};
                    client_ev.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP;
                    client_ev.data.fd = client_fd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_ev) < 0) {
                        log_message("[error] epoll_ctl ADD client_fd failed");
                        close(client_fd);
                        continue;
                    }

                    buffers[client_fd] = "";
                    stats_inc_connection();
                    log_message("[connect] client_fd=" + std::to_string(client_fd));
                }
            } else {
                if (evs & (EPOLLHUP | EPOLLRDHUP)) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                    close(fd);
                    buffers.erase(fd);
                    stats_dec_connection();
                    log_message("[disconnect] client_fd=" + std::to_string(fd));
                    continue;
                }

                if (evs & EPOLLIN) {
                    bool closed = false;
                    while (true) {
                        char buf[1024];
                        ssize_t count = read(fd, buf, sizeof(buf));
                        if (count > 0) {
                            buffers[fd].append(buf, buf + count);
                            std::string& data = buffers[fd];
                            std::size_t pos;
                            while ((pos = data.find('\n')) != std::string::npos) {
                                std::string line = data.substr(0, pos);
                                data.erase(0, pos + 1);

                                pool.Enqueue([fd, line]() {
                                    std::string response = handle_command(line);
                                    send(fd, response.c_str(), response.size(), 0);
                                });

                                log_message("[cmd] fd=" + std::to_string(fd) + " line=" + line);
                            }
                        } else if (count == 0) {
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                            close(fd);
                            buffers.erase(fd);
                            stats_dec_connection();
                            log_message("[disconnect] client_fd=" + std::to_string(fd));
                            closed = true;
                            break;
                        } else {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                break;
                            } else {
                                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                                close(fd);
                                buffers.erase(fd);
                                stats_dec_connection();
                                log_message("[error] read failed, disconnect fd=" + std::to_string(fd));
                                closed = true;
                                break;
                            }
                        }
                    }
                    if (closed) continue;
                }
            }
        }
    }

    close(epoll_fd);
    close(server_fd);
    return 0;
}
