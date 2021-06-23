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

#include <amqpprox_testsocketstate.h>

#include <iostream>

namespace Bloomberg {
namespace amqpprox {

void TestSocketState::recordCall(const std::string &call)
{
    d_stateMachine[d_currentStep].d_record.push_back(Call(call));
}

void TestSocketState::recordCallCheck(const std::string &call, ErrorCode &ec)
{
    d_stateMachine[d_currentStep].d_record.push_back(Call(call));
}

TestSocketState::State &TestSocketState::currentState()
{
    return d_currentState;
}

std::size_t
TestSocketState::recordData(const void *data, std::size_t len, ErrorCode &ec)
{
    auto dataptr = static_cast<const uint8_t *>(data);
    Data toRecord(std::vector<uint8_t>(dataptr, dataptr + len), ec);
    d_stateMachine[d_currentStep].d_record.push_back(toRecord);
    // TODO Enhancement: this might want to indicate a partial write
    return len;
}

void TestSocketState::handleTransition(std::function<void(Item *)> handler)
{
    d_handler = handler;
}

void TestSocketState::setCurrentStep(int step)
{
    d_currentStep = step;
}

bool TestSocketState::drive()
{
    auto &step    = d_stateMachine[d_currentStep];
    auto  sz      = step.d_produce.size();
    bool  didWork = false;

    for (; step.d_produceIndex < sz; ++step.d_produceIndex) {
        auto item = &step.d_produce[step.d_produceIndex];
        didWork   = true;

        if (auto state = std::get_if<State>(item)) {
            d_currentState = *state;
        }
        else if (auto func = std::get_if<Func>(item)) {
            (*func)();
        }
        else {
            d_handler(item);
        }
    }

    return didWork;
}

void TestSocketState::validate()
{
    auto &step = d_stateMachine[d_currentStep];
    if (step.d_validator) {
        step.d_validator(step.d_record);
    }
}

void TestSocketState::pushItem(int step, const Item &item)
{
    d_stateMachine[step].d_produce.push_back(item);
}

void TestSocketState::expect(int step, Validator validator)
{
    d_stateMachine[step].d_validator = validator;
}

// Free functions

bool operator==(const TestSocketState::Data &lhs,
                const TestSocketState::Data &rhs)
{
    return lhs.d_ec == rhs.d_ec && lhs.d_value == rhs.d_value;
}

bool operator==(const TestSocketState::Call &lhs,
                const TestSocketState::Call &rhs)
{
    return lhs.d_value == rhs.d_value;
}

std::ostream &operator<<(std::ostream &os, const TestSocketState::Data &data)
{
    os << "Data[ec=" << data.d_ec << ", len=" << data.d_value.size() << "]";
    return os;
}
std::ostream &operator<<(std::ostream &os, const TestSocketState::Call &data)
{
    os << "Call[" << data.d_value << "]";
    return os;
}

std::ostream &operator<<(std::ostream &os, const TestSocketState::State &data)
{
    os << "State[local=" << data.d_local << ", remote=" << data.d_remote
       << ", secure=" << data.d_secure << "]";
    return os;
}
std::ostream &operator<<(std::ostream &os, const TestSocketState::Error &data)
{
    os << "Error[]";
    return os;
}

std::ostream &operator<<(std::ostream &os, const TestSocketState::Func &data)
{
    os << "Func[]";
    return os;
}
std::ostream &operator<<(std::ostream &                            os,
                         const TestSocketState::HandshakeComplete &data)
{
    os << "HandshakeComplete[" << data.d_ec << "]";
    return os;
}

std::ostream &operator<<(std::ostream &                        os,
                         const TestSocketState::WriteComplete &data)
{
    os << "WriteComplete[" << data.d_ec << "]";
    return os;
}
std::ostream &operator<<(std::ostream &                          os,
                         const TestSocketState::ConnectComplete &data)
{
    os << "ConnectComplete[" << data.d_ec << "]";
    return os;
}

}
}
