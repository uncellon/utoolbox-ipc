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

#ifndef UT_IPC_SERVER_H
#define UT_IPC_SERVER_H

#define UT_IPC_BACKLOG 16

#include "ut/ipc/common.h"

#include <poll.h>
#include <ut/core/event.h>

namespace UT {

class IPCServer {
public:
    enum class RetCode;

    /**************************************************************************
     * Constructors / Destructors
     *************************************************************************/

    IPCServer() = default;
    IPCServer(const IPCServer&) = delete;
    IPCServer(IPCServer&&) = delete;
    ~IPCServer();

    /**************************************************************************
     * Methods
     *************************************************************************/

    void send(int to, const void* data, size_t bytes);
    RetCode start(const std::string& name);
    RetCode stop();

    /**************************************************************************
     * Events
     *************************************************************************/

    Event<int> onClientConnected;
    Event<int> onClientDisconnected;
    Event<int, std::shared_ptr<void>, ssize_t> onDataReceived;

protected:
    /**************************************************************************
     * Methods
     *************************************************************************/

    void loop();

    /**************************************************************************
     * Members
     *************************************************************************/

    bool mRunning = false;
    int mPipe[2];
    int mSfd = 0;
    std::mutex mMutex;
    std::string mServerName;
    std::thread* mThread = nullptr;
    std::vector<pollfd> mPfds;
    void* mBuffer = nullptr;
}; // class IPCServer

enum class IPCServer::RetCode {
    kSuccess,
    kAlreadyStarted,
    kNotStarted
}; // IPCServer::RetCode

} // namespace UT

#endif // UT_IPC_SERVER_H