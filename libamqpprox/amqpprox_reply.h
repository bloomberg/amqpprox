/*
** Copyright 2021 Bloomberg Finance L.P.
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
#ifndef BLOOMBERG_AMQPPROX_REPLY
#define BLOOMBERG_AMQPPROX_REPLY

#include <cstdint>

namespace Bloomberg {
namespace amqpprox {

namespace Reply {

/// All reply codes per AMQP 0.9.1 specification,
/// https://www.rabbitmq.com/amqp-0-9-1-reference.html#domain.reply-code
struct Codes {
    enum {
        reply_success       = 200,
        content_too_large   = 311,
        no_consumers        = 313,
        connection_forced   = 320,
        invalid_path        = 402,
        access_refused      = 403,
        not_found           = 404,
        resource_locked     = 405,
        precondition_failed = 406,
        frame_error         = 501,
        syntax_error        = 502,
        command_invalid     = 503,
        channel_error       = 504,
        unexpected_frame    = 505,
        resource_error      = 506,
        not_allowed         = 530,
        not_implemented     = 540,
        internal_error      = 541
    };
};

struct OK {
    constexpr static const uint16_t    CODE = Codes::reply_success;
    constexpr static const char *const TEXT = "OK";
};

struct CloseOkExpected {
    constexpr static const uint16_t    CODE = Codes::channel_error;
    constexpr static const char *const TEXT = "ERROR: Expected CloseOk reply";
};

}
}
}

#endif  // !BLOOMBERG_AMQPPROX_REPLY
