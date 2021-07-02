#ifndef BLOOMBERG_AMQPPROX_CONSTANTS
#define BLOOMBERG_AMQPPROX_CONSTANTS

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Represents global constants used by different classes
 */
class Constants {
  public:
    static constexpr const char *protocolHeader()
    {
        return "AMQP\x00\x00\x09\x01";
    }

    static constexpr std::size_t protocolHeaderLength() { return 8; }

    static constexpr const char *legacyProtocolHeader()
    {
        return "AMQP\x01\x01\x00\x09";
    }

    static constexpr std::size_t legacyProtocolHeaderLength() { return 8; }

    static constexpr const char *proxyProtocolV1Identifier()
    {
        return "PROXY";
    }

    static constexpr const char *inetTcp4() { return "TCP4"; }

    static constexpr const char *inetTcp6() { return "TCP6"; }

    static constexpr const char *inetUnknown() { return "UNKNOWN"; }

    static constexpr const char *proxyProtocolV1Enabled()
    {
        return "PROXY PROTOCOL V1 ENABLED";
    }

    static constexpr const char *sendProxy() { return "SEND-PROXY"; }

    static constexpr const char *tlsCommand() { return "TLS"; }

    static constexpr int versionMajor() { return 0; }

    static constexpr int versionMinor() { return 9; }

    static constexpr const char *authenticationMechanism() { return "PLAIN"; }

    static constexpr const char *locale() { return "en_US"; }

    static constexpr int channelMaximum() { return 2047; }

    static constexpr int maxFrameSize() { return 131072; }

    static constexpr int defaultHeartbeatInterval() { return 60; }

    static constexpr const char *copyrightNotice()
    {
        return "Copyright (C) Bloomberg Finance L.P. 2016.";
    }

    static constexpr const char *product() { return "amqpprox"; }

    static constexpr const char *version() { return "0.1"; }

    static constexpr const char *clusterName() { return "amqpprox_cloud"; }

    static constexpr const char *cr() { return "\r\n"; }

    static constexpr uint8_t frameEnd() { return 0xCE; }
};

}
}

#endif
