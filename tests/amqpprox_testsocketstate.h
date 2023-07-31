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

#ifndef BLOOMBERG_AMQPPROX_TESTSOCKETSTATE
#define BLOOMBERG_AMQPPROX_TESTSOCKETSTATE

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include <cstddef>
#include <ostream>
#include <unordered_map>
#include <variant>

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Provide test data for a socket
 *
 * This component acts to provide test data for a socket to read from and
 * storage of what happens on the socket interface in terms of data and calls.
 * This is organised into a number of steps, and in each step multiple pieces
 * of data can be produced onto the socket, or recorded as writes into the
 * socket. As such this component behaves like a simple state machine, first
 * you initialise its state for each step and then drive the state machine
 * linearly through the steps and sub-steps.
 */
class TestSocketState {
  public:
    using endpoint  = boost::asio::ip::tcp::endpoint;
    using ErrorCode = boost::system::error_code;

    // Helper classes for building the Item subtypes. These are always fallible
    // and so inherit at least from Result, but when additionally responsible
    // for some value they can inherit from Value templated on the type they
    // require.
    struct Result {
        ErrorCode d_ec{};

        Result() = default;
        Result(ErrorCode ec)
        : d_ec(ec)
        {
        }
    };

    template <typename MEMB>
    struct Value : Result {
        using Result::Result;

        MEMB d_value{};

        Value() = default;

        Value(const MEMB &value)
        : d_value(value)
        {
        }

        Value(const MEMB &value, ErrorCode ec)
        : Result(ec)
        , d_value(value)
        {
        }
    };

    // Subtypes of Item
    // NB these are not just type aliases because having them as different
    // types but with potentially the same shape makes using std::variant
    // easier.
    struct Data : Value<std::vector<uint8_t>> {
        using Value::Value;
    };
    struct Error : Value<std::string> {
        using Value::Value;
    };
    struct HandshakeComplete : Result {
        using Result::Result;
    };
    struct ConnectComplete : Result {
        using Result::Result;
    };
    struct WriteComplete : Value<std::size_t> {
        using Value::Value;
    };
    struct Call : Value<std::string> {
        using Value::Value;
    };

    struct State {
        endpoint d_local;
        endpoint d_remote;
        bool     d_secure;
    };

    using Func = std::function<void(void)>;

    // Item variant
    using Item = std::variant<Data,
                              State,
                              Call,
                              Error,
                              HandshakeComplete,
                              WriteComplete,
                              ConnectComplete,
                              Func>;

    using Validator = std::function<void(const std::vector<Item> &a)>;

  private:
    struct Step {
        int               d_produceIndex{0};
        std::vector<Item> d_produce;
        std::vector<Item> d_record;
        Validator         d_validator;
    };

    std::unordered_map<int, Step> d_stateMachine;
    int                           d_currentStep;
    State                         d_currentState;
    std::function<void(Item *)>   d_handler;

  public:
    /**
     * \brief Register handler for item changes
     *
     * This method registers a handler to receive the asynchronous item
     * changes, this is effectively simulating the new data becoming available
     * asynchronously in the calling code via this callback, but is initiated
     * synchronously by this component's `drive` method.
     */
    void handleTransition(std::function<void(Item *)> handler);

    /**
     * \brief Set the current step/state that the component is acting on
     *
     * This is expected to just be progressed linearly, nothing prevents it in
     * case the program wants to do the validation separately to the program
     * being driven.
     *
     * \param step The step we're acting on
     */
    void setCurrentStep(int step);

    /**
     * \brief Drive the state at the current step to completion
     */
    bool drive();

    /**
     * \brief Validate the current step
     *
     * Note that this returns void, the handlers must register their failure
     * via a passed in or global context. This is normal for Google Test macros
     * such as `EXPECT_*`.
     */
    void validate();

    /**
     * \brief Record that a function has been called (by name)
     * \param call The stringified name of the function being called.
     */
    void recordCall(const std::string &call);

    /**
     * \brief Record a function has been called and possibly set return code
     *
     * This is a simple extension of recordCall that also gives the opportunity
     * for this component to overwrite the value of the error code to simulate
     * a particular socket call failing.
     */
    void recordCallCheck(const std::string &call, ErrorCode &ec);

    /**
     * \brief Record some data into the test state for later validation
     * \param data A pointer to the data being recorded
     * \param len The length in bytes of the data being recorded
     * \param ec A mutable reference to the error code which can be set to
     *           indicate a write failure.
     * \return the number of bytes written to communicate a partial write
     */
    std::size_t recordData(const void *data, std::size_t len, ErrorCode &ec);

    /**
     * \brief Stage an `Item` for a particular step
     * \param step The state step the item is for
     * \param item The Item to be driven for that state
     */
    void pushItem(int step, const Item &item);

    /**
     * \brief Return a mutable reference to the current State of the socket
     * \return The current state
     */
    State &currentState();

    /**
     * \brief Install a validation functor on a particular state step
     * \param step The state step the item is for
     * \param func The validation functor
     */
    void expect(int step, Validator validator);
};

bool operator==(const TestSocketState::Data &lhs,
                const TestSocketState::Data &rhs);

bool operator==(const TestSocketState::Call &lhs,
                const TestSocketState::Call &rhs);

std::ostream &operator<<(std::ostream &os, const TestSocketState::Data &data);
std::ostream &operator<<(std::ostream &os, const TestSocketState::Call &data);
std::ostream &operator<<(std::ostream &os, const TestSocketState::State &data);
std::ostream &operator<<(std::ostream &os, const TestSocketState::Error &data);
std::ostream &operator<<(std::ostream &os, const TestSocketState::Func &data);
std::ostream &operator<<(std::ostream                             &os,
                         const TestSocketState::HandshakeComplete &data);
std::ostream &operator<<(std::ostream                         &os,
                         const TestSocketState::WriteComplete &data);
std::ostream &operator<<(std::ostream                           &os,
                         const TestSocketState::ConnectComplete &data);

}
}

#endif
