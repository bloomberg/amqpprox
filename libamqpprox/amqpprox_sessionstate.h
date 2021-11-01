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

/**
 * \brief Maintains the state and metrics of a particular session
 */
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
    std::atomic<bool>               d_authDeniedConnection;
    std::atomic<bool>               d_ingressSecured;
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
    explicit SessionState(
        const std::shared_ptr<HostnameMapper> &hostnameMapper = nullptr);

    // MANIPULATORS
    /**
     * \brief Set the tcp pair for egress
     * \param ioService handle to the boost asio service
     * \param local local endpoint
     * \param remote remote endpoint
     */
    void setEgress(boost::asio::io_service &      ioService,
                   boost::asio::ip::tcp::endpoint local,
                   boost::asio::ip::tcp::endpoint remote);

    /**
     * \brief Set the tcp pair for ingress
     * \param ioService handle to the boost asio service
     * \param local local endpoint
     * \param remote remote endpoint
     */
    void setIngress(boost::asio::io_service &      ioService,
                    boost::asio::ip::tcp::endpoint local,
                    boost::asio::ip::tcp::endpoint remote);

    /**
     * \brief Set the virtual host
     * \param vhost virtual host
     */
    void setVirtualHost(const std::string &vhost);

    /**
     * \brief Set the virtual host to be paused state
     * \param paused flag to specify paused or unpaused virtual host
     */
    void setPaused(bool paused);

    /**
     * \brief Set the denied connection flag, because of auth failure
     * \param authDenied flag to specify denied connection because of auth
     * failure
     */
    void setAuthDeniedConnection(bool authDenied);

    /**
     * \brief Set the ingress secured connection flag
     * \param secured flag to specify the ingress connection is secured (TLS
     * enabled)
     */
    void setIngressSecured(bool secured);

    /**
     * \brief Set session as disconnected, along with which type of disconnect
     * \param disconnectType specifies type of disconnection
     */
    void setDisconnected(DisconnectType disconnectType);

    /**
     * \brief Set up a hostname mapper for the Session
     * \param ioService handle to the boost asio service
     * \param hostnameMapper shared pointer to `HostnameMapper`
     */
    void
    setHostnameMapper(boost::asio::io_service &              ioService,
                      const std::shared_ptr<HostnameMapper> &hostnameMapper);

    /**
     * \brief Increment frames and bytes totals for ingress
     * \param frames number of frames
     * \param bytes number of bytes
     */
    void incrementIngressTotals(uint64_t frames, uint64_t bytes);

    /**
     * \brief Increment frames and bytes totals for egress
     * \param frames number of frames
     * \param bytes number of bytes
     */
    void incrementEgressTotals(uint64_t frames, uint64_t bytes);

    /**
     * \brief Add a latency value for ingress
     * \param latency in milliseconds
     */
    void addIngressLatency(uint64_t latency);

    /**
     * \brief Add a latency value for egress
     * \param latency in milliseconds
     */
    void addEgressLatency(uint64_t latency);

    /**
     * \brief Get the hostname for the endpoint
     * \param endpoint ip address
     */
    std::string hostname(const boost::asio::ip::tcp::endpoint &endpoint) const;

    // ACCESSORS
    /**
     * \return the egress endpoints
     */
    inline EndpointPair getEgress() const;

    /**
     * \return the ingress endpoints
     */
    inline EndpointPair getIngress() const;

    /**
     * \return the virtual host
     */
    inline const std::string &getVirtualHost() const;

    /**
     * \return the state(paused/unpaused) of the virtual host
     */
    inline bool getPaused() const;

    /**
     * \return the state of the connection, whether it is denied because of
     * auth failure
     */
    inline bool getAuthDeniedConnection() const;

    /**
     * \return the state of the ingress connection, whether it is secured (TLS
     * enabled)
     */
    inline bool getIngressSecured() const;

    /**
     * \return session identifier
     */
    inline uint64_t id() const;

    /**
     * \brief Get values of all the maintained metrics
     */
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

    /**
     * \return whether session is disconnected and type of disconnection
     */
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

inline bool SessionState::getAuthDeniedConnection() const
{
    return d_authDeniedConnection;
}

inline bool SessionState::getIngressSecured() const
{
    return d_ingressSecured;
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
