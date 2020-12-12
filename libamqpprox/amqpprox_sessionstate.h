/*
** Copyright 2020 Bloomberg Finance L.P.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
#ifndef BLOOMBERG_AMQPPROX_SESSIONSTATE
#define BLOOMBERG_AMQPPROX_SESSIONSTATE

#include <boost/asio.hpp>

#include <atomic>
#include <iosfwd>
#include <memory>
#include <mutex>
#include <string>

namespace Bloomberg {
namespace amqpprox {

class HostnameMapper;

class SessionState {
  public:
    enum class DisconnectType {
        NOT_DISCONNECTED = 0,
        DISCONNECTED_CLEANLY,
        DISCONNECTED_CLIENT,
        DISCONNECTED_SERVER,
        DISCONNECTED_PROXY
    };

  private:
    static uint64_t                 s_nextId;
    boost::asio::ip::tcp::endpoint  d_ingressLocalEndpoint;
    boost::asio::ip::tcp::endpoint  d_ingressRemoteEndpoint;
    boost::asio::ip::tcp::endpoint  d_egressLocalEndpoint;
    boost::asio::ip::tcp::endpoint  d_egressRemoteEndpoint;
    std::atomic<uint64_t>           d_ingressBytesTotal;
    std::atomic<uint64_t>           d_egressBytesTotal;
    std::atomic<uint64_t>           d_ingressPacketTotal;
    std::atomic<uint64_t>           d_egressPacketTotal;
    std::atomic<uint64_t>           d_ingressFrameTotal;
    std::atomic<uint64_t>           d_egressFrameTotal;
    std::atomic<uint64_t>           d_ingressLatencyCount;
    std::atomic<uint64_t>           d_egressLatencyCount;
    std::atomic<uint64_t>           d_ingressLatencyTotal;
    std::atomic<uint64_t>           d_egressLatencyTotal;
    std::atomic<bool>               d_paused;
    std::string                     d_virtualHost;
    DisconnectType                  d_disconnectedStatus;
    uint64_t                        d_id;
    mutable std::mutex              d_lock;
    std::shared_ptr<HostnameMapper> d_hostnameMapper;

  public:
    typedef std::pair<boost::asio::ip::tcp::endpoint,
                      boost::asio::ip::tcp::endpoint>
        EndpointPair;

    // CREATORS
    SessionState(
        const std::shared_ptr<HostnameMapper> &hostnameMapper = nullptr);

    // MANIPULATORS
    void setEgress(boost::asio::io_service &      ioService,
                   boost::asio::ip::tcp::endpoint local,
                   boost::asio::ip::tcp::endpoint remote);
    ///< Set the tcp pair for egress

    void setIngress(boost::asio::io_service &      ioService,
                    boost::asio::ip::tcp::endpoint local,
                    boost::asio::ip::tcp::endpoint remote);
    ///< Set the tcp pair for ingress

    void setVirtualHost(const std::string &vhost);
    ///< Set the virtual host

    void setPaused(bool paused);
    ///< Set the virtual host to be paused state

    void setDisconnected(DisconnectType disconnectType);
    ///< Set session as disconnected, along with which type of disconnect

    void
    setHostnameMapper(boost::asio::io_service &              ioService,
                      const std::shared_ptr<HostnameMapper> &hostnameMapper);
    ///< Set up a hostname mapper for the Session

    void incrementIngressTotals(uint64_t frames, uint64_t bytes);
    ///< Increment frames and bytes totals for ingress

    void incrementEgressTotals(uint64_t frames, uint64_t bytes);
    ///< Increment frames and bytes totals for egress

    void addIngressLatency(uint64_t latency);
    ///< Add a latency value for ingress

    void addEgressLatency(uint64_t latency);
    ///< Add a latency value for egress

    std::string hostname(const boost::asio::ip::tcp::endpoint &endpoint) const;
    ///< Get the hostname for the endpoint

    // ACCESSORS
    inline EndpointPair getEgress() const;
    ///< Get the egress endpoints

    inline EndpointPair getIngress() const;
    ///< Get the ingress endpoints

    inline const std::string &getVirtualHost() const;

    inline bool getPaused() const;

    inline uint64_t id() const;

    void getTotals(uint64_t *ingressPackets,
                   uint64_t *ingressFrames,
                   uint64_t *ingressBytes,
                   uint64_t *ingressLatencyTotalMs,
                   uint64_t *ingressLatencyCount,
                   uint64_t *egressPackets,
                   uint64_t *egressFrames,
                   uint64_t *egressBytes,
                   uint64_t *egressLatencyTotalMs,
                   uint64_t *egressLatencyCount) const;

    inline DisconnectType getDisconnectType() const;
};

inline SessionState::EndpointPair SessionState::getEgress() const
{
    std::lock_guard<std::mutex> lg(d_lock);
    return std::make_pair(d_egressLocalEndpoint, d_egressRemoteEndpoint);
}

inline SessionState::EndpointPair SessionState::getIngress() const
{
    std::lock_guard<std::mutex> lg(d_lock);
    return std::make_pair(d_ingressLocalEndpoint, d_ingressRemoteEndpoint);
}

inline const std::string &SessionState::getVirtualHost() const
{
    std::lock_guard<std::mutex> lg(d_lock);
    return d_virtualHost;
}

inline bool SessionState::getPaused() const
{
    return d_paused;
}

inline uint64_t SessionState::id() const
{
    return d_id;
}

inline SessionState::DisconnectType SessionState::getDisconnectType() const
{
    std::lock_guard<std::mutex> lg(d_lock);
    return d_disconnectedStatus;
}

std::ostream &operator<<(std::ostream &os, const SessionState &state);

}
}

#endif
