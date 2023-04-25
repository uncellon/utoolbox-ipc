/******************************************************************************
 * 
 * Copyright (C) 2023 Dmitry Plastinin
 * Contact: uncellon@yandex.ru, uncellon@gmail.com, uncellon@mail.ru
 * 
 * This file is part of the UT IPC library.
 * 
 * UT IPC is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as pubblished by the
 * Free Software Foundation, either version 3 of the License, or (at your 
 * option) any later version.
 * 
 * UT IPC is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser Public License for more
 * details
 * 
 * You should have received a copy of the GNU Lesset General Public License
 * along with UT IPC. If not, see <https://www.gnu.org/licenses/>.
 * 
 *****************************************************************************/

#include "server.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace UT {

/******************************************************************************
 * Constructors / Destructors
 *****************************************************************************/

IPCServer::~IPCServer() {
    stop();
}

/******************************************************************************
 * Methods
 *****************************************************************************/

void IPCServer::send(int to, const void* data, size_t bytes) {
    ::send(to, data, bytes, 0);
}

IPCServer::RetCode IPCServer::start(const std::string& name) {
    std::unique_lock lock(mMutex);

    if (mRunning) {
        return RetCode::kAlreadyStarted;
    }

    mServerName = UT_IPC_SOCKET_PATH + name;

    // Allocate buffer
    mBuffer = malloc(UT_IPC_BUFFER_SIZE);
    if (!mBuffer) {
        throw std::runtime_error("malloc(...) failed, errno: " + std::to_string(errno));
    }

    // Initialize pipes
    int ret = pipe(mPipe);
    if (ret == -1) {
        free(mBuffer);
        mBuffer = nullptr;
        throw std::runtime_error("pipe(...) failed, errno: " + std::to_string(errno));
    }

    ret = fcntl(mPipe[0], F_GETFL, nullptr);
    if (ret == -1) {
        close(mPipe[0]);
        close(mPipe[1]);
        free(mBuffer);
        mPipe[0] = 0;
        mPipe[1] = 0;
        mBuffer = nullptr;
        throw std::runtime_error("fcntl(..., F_GETFL, ...) failed, errno: " + std::to_string(errno));
    }

    ret = fcntl(mPipe[0], F_SETFL, ret |= O_NONBLOCK);
    if (ret == -1) {
        close(mPipe[0]);
        close(mPipe[1]);
        free(mBuffer);
        mPipe[0] = 0;
        mPipe[1] = 0;
        mBuffer = nullptr;
        throw std::runtime_error("fcntl(..., F_SETFL, ...) failed, errno: " + std::to_string(errno));
    }

    pollfd pfd;
    pfd.fd = mPipe[0];
    pfd.events = POLLIN;
    mPfds.push_back(pfd);

    // Initialize socket
    mSfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (mSfd == -1) {
        close(mPipe[0]);
        close(mPipe[1]);
        free(mBuffer);
        mPipe[0] = 0;
        mPipe[1] = 0;
        mBuffer = nullptr;
        throw std::runtime_error("socket(...) failed, errno: " + std::to_string(errno));
    }

    sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));

    if (remove(mServerName.c_str()) == -1 && errno != ENOENT) {
        close(mSfd);
        close(mPipe[0]);
        close(mPipe[1]);
        free(mBuffer);
        mSfd = 0;
        mPipe[0] = 0;
        mPipe[1] = 0;
        mBuffer = nullptr;
        throw std::runtime_error("remove(...) failed, errno: " + std::to_string(errno));
    }

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, mServerName.c_str(), sizeof(addr.sun_path));

    ret = bind(mSfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (ret == -1) {
        close(mSfd);
        close(mPipe[0]);
        close(mPipe[1]);
        free(mBuffer);
        mSfd = 0;
        mPipe[0] = 0;
        mPipe[1] = 0;
        mBuffer = nullptr;
        throw std::runtime_error("bind(...) failed, errno: " + std::to_string(errno));
    }

    ret = listen(mSfd, UT_IPC_BACKLOG);
    if (ret == -1) {
        close(mSfd);
        close(mPipe[0]);
        close(mPipe[1]);
        free(mBuffer);
        mSfd = 0;
        mPipe[0] = 0;
        mPipe[1] = 0;
        mBuffer = nullptr;
        throw std::runtime_error("listen(...) failed, errno: " + std::to_string(errno));
    }

    ret = fcntl(mSfd, F_GETFL, nullptr);
    if (ret == -1) {
        close(mSfd);
        close(mPipe[0]);
        close(mPipe[1]);
        free(mBuffer);
        mSfd = 0;
        mPipe[0] = 0;
        mPipe[1] = 0;
        mBuffer = nullptr;
        throw std::runtime_error("fcntl(..., F_GETFL, ...) failed, errno: " + std::to_string(errno));
    }

    ret = fcntl(mSfd, F_SETFL, ret |= O_NONBLOCK);
    if (ret == -1) {
        close(mSfd);
        close(mPipe[0]);
        close(mPipe[1]);
        free(mBuffer);
        mSfd = 0;
        mPipe[0] = 0;
        mPipe[1] = 0;
        mBuffer = nullptr;
        throw std::runtime_error("fcntl(..., F_SETFL, ...) failed, errno: " + std::to_string(errno));
    }
   
    pfd.fd = mSfd;
    mPfds.push_back(pfd);

    mRunning = true;
    mThread = new std::thread(&IPCServer::loop, this);

    return RetCode::kSuccess;
}

IPCServer::RetCode IPCServer::stop() {
    if (!mRunning) {
        return RetCode::kNotStarted;
    }

    mRunning = false;

    char code = '0';
    write(mPipe[1], &code, sizeof(code));
    mThread->join();
    delete mThread;    
    mThread = nullptr;

    for (size_t i = 0; i < mPfds.size(); ++i) {
        close(mPfds[i].fd);
    }
    mPfds.clear();

    close(mSfd);
    close(mPipe[0]);
    close(mPipe[1]);
    free(mBuffer);
    mSfd = 0;
    mPipe[0] = 0;
    mPipe[1] = 0;
    mBuffer = nullptr;

    return RetCode::kSuccess;
}

void IPCServer::loop() {
    int ret = 0;
    ssize_t bytes = 0;

    while (mRunning) {
        ret = poll(mPfds.data(), mPfds.size(), -1);

        if (ret > 0) {
            // Software interrupt by pipe
            if (mPfds[0].revents & POLLIN) {
                mPfds[0].revents = 0;
                read(mPipe[0], mBuffer, UT_IPC_BUFFER_SIZE);
            }

            // New connection accept
            if (mPfds[1].revents & POLLIN) {
                mPfds[1].revents = 0;

                int cfd = accept(mSfd, nullptr, nullptr);
                if (cfd == -1) {
                    continue;
                }

                ret = fcntl(cfd, F_GETFL, nullptr);
                if (ret == -1) {
                    close(cfd);
                    continue;
                }

                ret = fcntl(cfd, F_SETFL, ret |= O_NONBLOCK);
                if (ret == -1) {
                    close(cfd);
                    continue;
                }

                pollfd pfd;
                pfd.fd = cfd;
                pfd.events = POLLIN;
                mPfds.push_back(pfd);

                onClientConnected(cfd);
            }

            // Process data from clients
            for (size_t i = 2; i < mPfds.size(); ++i) {
                if (!mPfds[i].revents & POLLIN) {
                    continue;
                }
                mPfds[i].revents = 0;           

                bytes = 0;
                void* data = nullptr;
                while (true) {
                    ret = recv(mPfds[i].fd, mBuffer, UT_IPC_BUFFER_SIZE, 0);

                    if (ret > 0) {
                        data = realloc(data, bytes + ret);
                        memcpy(static_cast<char*>(data) + bytes, mBuffer, ret);
                        bytes += ret;
                    } else if (ret == 0) { // Connection closed
                        onClientDisconnected(mPfds[i].fd);
                        close(mPfds[i].fd);
                        mPfds.erase(mPfds.begin() + i);
                        --i;
                        break;
                    } else if (ret == -1) {
                        break;
                    }
                }

                if (bytes) {
                    onDataReceived(mPfds[i].fd, std::shared_ptr<void>(data, [] (void* data) { free(data); }), bytes);
                }
            }
        } else if (ret == 0) { // No events
            continue;
        } else if (ret == -1) { // Error occured
            if (errno == EINTR) {
                continue;
            }
            
            throw std::runtime_error("poll(...) failed, errno: " + std::to_string(errno));
        }
    }
}

} // namespace UT