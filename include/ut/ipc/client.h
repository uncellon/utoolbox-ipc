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

#ifndef UT_IPC_CLIENT_H
#define UT_IPC_CLIENT_H

#define UT_IPC_RECONNECT_TIMEOUT 10

#include "ut/ipc/common.h"

#include <poll.h>
#include <ut/core/event.h>

namespace UT {

class IPCClient {
public:
    enum class RetCode;

    /**************************************************************************
     * Constructors / Destructors
     *************************************************************************/

    IPCClient() = default;
    IPCClient(const IPCClient&) = delete;
    IPCClient(IPCClient&&) = delete;
    ~IPCClient();

    /**************************************************************************
     * Methods
     *************************************************************************/

    void send(const void* data, size_t bytes);
    RetCode start(const std::string& server);
    RetCode stop();

    /**************************************************************************
     * Accessors / Mutators
     *************************************************************************/

    bool getReady() const;

    unsigned int getReconnectTimeout() const;
    void setReconnectTimeout(unsigned int timeout);

    /**************************************************************************
     * Events
     *************************************************************************/

    Event<bool> onReadyChanged;
    Event<std::shared_ptr<void>, ssize_t> onDataReceived;

protected:
    /**************************************************************************
     * Methods (Protected)
     *************************************************************************/

    void loop(const std::string& server);

    /**************************************************************************
     * Members
     *************************************************************************/

    bool mReady = false;
    bool mRunning = false;
    int mPipe[2];
    int mSfd = 0;
    pollfd mPfds[2];
    std::mutex mMutex;
    std::string mServerPath;
    std::thread* mThread = nullptr;
    unsigned int mReconnectTimeout = UT_IPC_RECONNECT_TIMEOUT;
    void* mBuffer = nullptr;
}; // class IPCClient

enum class IPCClient::RetCode {
    kSuccess,
    kAlreadyStarted,
    kNotStarted
}; // IPCClient::RetCode

/******************************************************************************
 * Inline Definition: Accessors / Mutators
 *****************************************************************************/

inline bool IPCClient::getReady() const { return mReady; }

inline unsigned int IPCClient::getReconnectTimeout() const { return mReconnectTimeout; }
inline void IPCClient::setReconnectTimeout(unsigned int timeout) { mReconnectTimeout = timeout; }

} // namespace UT

#endif // UT_IPC_CLIENT_H