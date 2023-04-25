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

#include "client.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>

namespace UT {

/******************************************************************************
 * Constructors / Destructors
 *****************************************************************************/

IPCClient::~IPCClient() {
    stop();
}

/******************************************************************************
 * Methods
 *****************************************************************************/

IPCClient::RetCode IPCClient::start(const std::string& server) {
    std::unique_lock lock(mMutex);

    if (mRunning) {
        return RetCode::kAlreadyStarted;
    }

    mServerPath = UT_IPC_SOCKET_PATH + server;

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

    mPfds[0].fd = mPipe[0];
    mPfds[0].events = POLLIN;

    mPfds[1].events = POLLIN;

    mRunning = true;
    mThread = new std::thread(&IPCClient::loop, this, mServerPath);

    return RetCode::kSuccess;
}

IPCClient::RetCode IPCClient::stop() {
    std::unique_lock lock(mMutex);

    if (!mRunning) {
        return RetCode::kNotStarted;
    }

    mRunning = false;
    onReadyChanged(mReady = false);
    char code = '0';
    write(mPipe[1], &code, sizeof(code));
    mThread->join();
    delete mThread;
    mThread = nullptr;

    close(mPipe[0]);
    close(mPipe[1]);
    mPipe[0] = 0;
    mPipe[1] = 0;

    free(mBuffer);
    mBuffer = nullptr;

    return RetCode::kSuccess;
}

void IPCClient::send(const void* data, size_t bytes) {
    ::send(mSfd, data, bytes, 0);
}

/******************************************************************************
 * Methods (Protected)
 *****************************************************************************/

void IPCClient::loop(const std::string& server) {
    int ret = 0;
    ssize_t bytesRead = 0;

    sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, server.c_str(), sizeof(addr.sun_path));

    while (mRunning) {
        mSfd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (mSfd == -1) {
            throw std::runtime_error("socket(...) failed, errno: " + std::to_string(errno));
        }

        ret = connect(mSfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        if (ret == -1) {
            sleep(mReconnectTimeout);
            continue;
        }

        ret = fcntl(mSfd, F_GETFL, nullptr);
        if (ret == -1) {
            throw std::runtime_error("fcntl(..., F_GETFL, ...) failed, errno: " + std::to_string(errno));
        }

        ret = fcntl(mSfd, F_SETFL, ret |= O_NONBLOCK);
        if (ret == -1) {
            throw std::runtime_error("fcntl(..., F_SETFL, ...) failed, errno: " + std::to_string(errno));
        }
           
        mPfds[1].fd = mSfd;

        onReadyChanged(mReady = true);
        while (mReady) {
            ret = poll(mPfds, 2, -1);

            if (ret > 0) {
                if (mPfds[0].revents & POLLIN) {
                    mPfds[0].revents = 0;
                    read(mPipe[0], mBuffer, UT_IPC_BUFFER_SIZE);
                }

                if (mPfds[1].revents & POLLIN) {
                    mPfds[1].revents = 0;

                    bytesRead = 0;
                    void* data = nullptr;
                    while (true) {
                        ret = recv(mPfds[1].fd, mBuffer, UT_IPC_BUFFER_SIZE, 0);

                        if (ret > 0) {
                            data = realloc(data, bytesRead + ret);
                            bytesRead += ret;
                            memcpy(static_cast<char*>(data) + bytesRead, mBuffer, ret);
                        } else if (ret == 0) { // Connection closed
                            close(mSfd);
                            mSfd = 0;
                            onReadyChanged(mReady = false);
                            break;
                        } else if (ret == -1) {
                            if (errno != EWOULDBLOCK) {
                                close(mSfd);
                                mSfd = 0;
                                onReadyChanged(mReady = false);
                            }
                            break;
                        }
                    }

                    if (bytesRead) {
                        onDataReceived(std::shared_ptr<void>(data, [] (void* data) { free(data); }), bytesRead);
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
}

} // namespace UT