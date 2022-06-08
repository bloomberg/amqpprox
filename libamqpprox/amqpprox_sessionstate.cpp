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
#include <amqpprox_sessionstate.h>

#include <amqpprox_hostnamemapper.h>

#include <iomanip>
#include <iostream>

#include <boost/lexical_cast.hpp>

namespace Bloomberg {
namespace amqpprox {

uint64_t SessionState::s_nextId = 1;

SessionState::SessionState(
    const std::shared_ptr<HostnameMapper> &hostnameMapper)
: d_ingressLocalEndpoint()
, d_ingressRemoteEndpoint()
, d_egressLocalEndpoint()
, d_egressRemoteEndpoint()
, d_ingressBytesTotal(0)
, d_egressBytesTotal(0)
, d_ingressPacketTotal(0)
, d_egressPacketTotal(0)
, d_ingressFrameTotal(0)
, d_egressFrameTotal(0)
, d_ingressLatencyTotal(0)
, d_ingressLatencyCount(0)
, d_egressLatencyTotal(0)
, d_egressLatencyCount(0)
, d_paused(false)
, d_readyToConnectOnUnpause(false)
, d_authDeniedConnection(false)
, d_ingressSecured(false)
, d_limitedConnection(false)
, d_virtualHost()
, d_disconnectedStatus(DisconnectType::NOT_DISCONNECTED)
, d_id(s_nextId++)  // This isn't a race because this is only on one thread
, d_lock()
, d_hostnameMapper(hostnameMapper)
{
}

void SessionState::setEgress(boost::asio::io_context       &ioContext,
                             boost::asio::ip::tcp::endpoint local,
                             boost::asio::ip::tcp::endpoint remote)
{
    if (d_hostnameMapper) {
        d_hostnameMapper->prime(ioContext, {local, remote});
    }

    std::lock_guard<std::mutex> lg(d_lock);
    d_egressLocalEndpoint  = local;
    d_egressRemoteEndpoint = remote;
}

void SessionState::setIngress(boost::asio::io_context       &ioContext,
                              boost::asio::ip::tcp::endpoint local,
                              boost::asio::ip::tcp::endpoint remote)
{
    if (d_hostnameMapper) {
        d_hostnameMapper->prime(ioContext, {local, remote});
    }

    std::lock_guard<std::mutex> lg(d_lock);
    d_ingressLocalEndpoint  = local;
    d_ingressRemoteEndpoint = remote;
}

void SessionState::setVirtualHost(const std::string &vhost)
{
    std::lock_guard<std::mutex> lg(d_lock);
    d_virtualHost = vhost;
}

void SessionState::setHostnameMapper(
    boost::asio::io_context               &ioContext,
    const std::shared_ptr<HostnameMapper> &hostnameMapper)
{
    if (!hostnameMapper) {
        return;
    }
    std::lock_guard<std::mutex> lg(d_lock);
    d_hostnameMapper = hostnameMapper;
    d_hostnameMapper->prime(ioContext,
                            {d_ingressLocalEndpoint,
                             d_ingressRemoteEndpoint,
                             d_egressLocalEndpoint,
                             d_egressRemoteEndpoint});
}

void SessionState::setPaused(bool paused)
{
    d_paused = paused;
}

void SessionState::setReadyToConnectOnUnpause(bool paused)
{
    d_readyToConnectOnUnpause = paused;
}

void SessionState::setAuthDeniedConnection(bool authDenied)
{
    d_authDeniedConnection = authDenied;
}

void SessionState::setIngressSecured(bool secured)
{
    d_ingressSecured = secured;
}

void SessionState::setDisconnected(SessionState::DisconnectType disconnect)
{
    std::lock_guard<std::mutex> lg(d_lock);
    d_disconnectedStatus = disconnect;
}

void SessionState::incrementIngressTotals(uint64_t frames, uint64_t bytes)
{
    d_ingressPacketTotal.fetch_add(1, std::memory_order_relaxed);
    d_ingressFrameTotal.fetch_add(frames, std::memory_order_relaxed);
    d_ingressBytesTotal.fetch_add(bytes, std::memory_order_relaxed);
}

void SessionState::incrementEgressTotals(uint64_t frames, uint64_t bytes)
{
    d_egressPacketTotal.fetch_add(1, std::memory_order_relaxed);
    d_egressFrameTotal.fetch_add(frames, std::memory_order_relaxed);
    d_egressBytesTotal.fetch_add(bytes, std::memory_order_relaxed);
}

void SessionState::addIngressLatency(uint64_t latency)
{
    d_ingressLatencyTotal.fetch_add(latency, std::memory_order_relaxed);
    d_ingressLatencyCount.fetch_add(1, std::memory_order_relaxed);
}

void SessionState::addEgressLatency(uint64_t latency)
{
    d_egressLatencyTotal.fetch_add(latency, std::memory_order_relaxed);
    d_egressLatencyCount.fetch_add(1, std::memory_order_relaxed);
}

void SessionState::setLimitedConnection()
{
    d_limitedConnection = true;
}

std::string
SessionState::hostname(const boost::asio::ip::tcp::endpoint &endpoint) const
{
    if (!d_hostnameMapper) {
        return boost::lexical_cast<std::string>(endpoint.address());
    }
    return d_hostnameMapper->mapToHostname(endpoint);
}

void SessionState::getTotals(uint64_t *ingressPackets,
                             uint64_t *ingressFrames,
                             uint64_t *ingressBytes,
                             uint64_t *ingressLatencyTotal,
                             uint64_t *ingressLatencyCount,
                             uint64_t *egressPackets,
                             uint64_t *egressFrames,
                             uint64_t *egressBytes,
                             uint64_t *egressLatencyTotal,
                             uint64_t *egressLatencyCount) const
{
    *ingressPackets = d_ingressPacketTotal.load(std::memory_order_relaxed);
    *ingressFrames  = d_ingressFrameTotal.load(std::memory_order_relaxed);
    *ingressBytes   = d_ingressBytesTotal.load(std::memory_order_relaxed);
    *egressPackets  = d_egressPacketTotal.load(std::memory_order_relaxed);
    *egressFrames   = d_egressFrameTotal.load(std::memory_order_relaxed);
    *egressBytes    = d_egressBytesTotal.load(std::memory_order_relaxed);
    *ingressLatencyTotal =
        d_ingressLatencyTotal.load(std::memory_order_relaxed);
    *ingressLatencyCount =
        d_ingressLatencyCount.load(std::memory_order_relaxed);
    *egressLatencyTotal = d_egressLatencyTotal.load(std::memory_order_relaxed);
    *egressLatencyCount = d_egressLatencyCount.load(std::memory_order_relaxed);
}

std::ostream &operator<<(std::ostream &os, const SessionState &state)
{
    uint64_t ingressPackets, ingressFrames, ingressBytes, ingressLatencyCount,
        ingressLatencyTotal, egressPackets, egressFrames, egressBytes,
        egressLatencyCount, egressLatencyTotal;

    state.getTotals(&ingressPackets,
                    &ingressFrames,
                    &ingressBytes,
                    &ingressLatencyTotal,
                    &ingressLatencyCount,
                    &egressPackets,
                    &egressFrames,
                    &egressBytes,
                    &egressLatencyTotal,
                    &egressLatencyCount);

    os << std::setw(7) << state.id() << ": "
       << "vhost=" << state.getVirtualHost() << " "
       << ", "
       << (state.getDisconnectType() ==
                   SessionState::DisconnectType::NOT_DISCONNECTED
               ? ""
               : "D")
       << (state.getPaused() ? "P " : " ")
       << (state.getAuthDeniedConnection() ? "DENY " : " ")
       << state.hostname(state.getIngress().second) << ":"
       << state.getIngress().second.port() << "->"
       << state.hostname(state.getIngress().first) << " --> "
       << state.hostname(state.getEgress().first) << ":"
       << state.getEgress().first.port() << "->"
       << state.hostname(state.getEgress().second) << ":"
       << state.getEgress().second.port() << " IN: " << ingressBytes << "B "
       << ingressFrames << " Frames in " << ingressPackets << " pkt. ";
    if (ingressLatencyCount > 0) {
        os << " Avg. Latency: " << ingressLatencyTotal / ingressLatencyCount
           << "ms ";
    }
    os << " OUT: " << egressBytes << "B " << egressFrames << " Frames in "
       << egressPackets << " pkt. ";
    if (egressLatencyCount > 0) {
        os << " Avg. Latency: " << egressLatencyTotal / egressLatencyCount
           << "ms ";
    }

    return os;
}

}
}
