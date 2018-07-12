// Copyright 2018 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _MICRORTPS_AGENT_CLIENT_SESSION_STREAM_OUTPUT_STREAM_HPP_
#define _MICRORTPS_AGENT_CLIENT_SESSION_STREAM_OUTPUT_STREAM_HPP_

#include <micrortps/agent/types/XRCETypes.hpp>
#include <micrortps/agent/utils/SeqNum.hpp>
#include <micrortps/agent/message/Message.hpp>

#include <vector>
#include <map>

// TODO (julian): move to global config.
#define MICRORTPS_RELIABLE_STREAM_DEPTH 16

namespace eprosima {
namespace micrortps {

/******************************************************************************
 * Output Stream.
 ******************************************************************************/
class OutputStream
{
public:
    OutputStream() {}
};

/******************************************************************************
 * Best-Effort Output Stream.
 ******************************************************************************/
class BestEffortOutputStream : public OutputStream
{
public:
    BestEffortOutputStream() : last_send_(~0) {}

    // TODO (julian): remove.
    SeqNum get_last_handled() const { return last_send_; }
    void promote_stream();

private:
    SeqNum last_send_;
};

inline void BestEffortOutputStream::promote_stream()
{
    last_send_ += 1;
}

/******************************************************************************
 * Reliable Output Stream.
 ******************************************************************************/
class ReliableOutputStream
{
public:
    ReliableOutputStream() : last_sent_(~0), last_acknown_(~0) {}

    ReliableOutputStream(const ReliableOutputStream&) = delete;
    ReliableOutputStream& operator=(const ReliableOutputStream) = delete;
    ReliableOutputStream(ReliableOutputStream&&);
    ReliableOutputStream& operator=(ReliableOutputStream&&);

    void push_message(const XrceMessage& message);
    XrceMessage get_message(SeqNum seq_num);
    void update_from_acknack(SeqNum first_unacked);
    SeqNum get_first_available() { return last_acknown_ + 1;}
    SeqNum get_last_available() { return last_sent_; }
    SeqNum next_message() { return last_sent_ + 1; }
    bool message_pending() { return messages_.size() != 0; }

private:
    SeqNum last_sent_;
    SeqNum last_acknown_;
    std::map<uint16_t, std::vector<uint8_t>> messages_;
};

inline ReliableOutputStream::ReliableOutputStream(ReliableOutputStream&& x)
    : last_sent_(x.last_sent_),
      last_acknown_(x.last_acknown_),
      messages_(std::move(messages_))
{
}

inline ReliableOutputStream& ReliableOutputStream::operator=(ReliableOutputStream&& x)
{
    last_sent_ = x.last_sent_;
    last_acknown_ = x.last_acknown_;
    messages_ = std::move(x.messages_);
    return *this;
}

inline void ReliableOutputStream::push_message(const XrceMessage& message)
{
    if (last_sent_ < last_acknown_ + 16)
    {
        last_sent_ += 1;
        messages_.emplace(std::piecewise_construct,
                          std::forward_as_tuple(last_sent_),
                          std::forward_as_tuple(message.buf, message.buf + message.len));
    }
}

inline XrceMessage ReliableOutputStream::get_message(SeqNum seq_num)
{
    XrceMessage message = {nullptr, 0};
    auto it = messages_.find(seq_num);
    if (it != messages_.end())
    {
        message.buf = reinterpret_cast<char*>(messages_.at(seq_num).data());
        message.len = messages_.at(seq_num).size();
    }
    return message;
}

inline void ReliableOutputStream::update_from_acknack(SeqNum first_unacked)
{
    while ((last_acknown_ + 1 < first_unacked) && (last_acknown_ < last_sent_))
    {
        last_acknown_ += 1;
        messages_.erase(last_acknown_);
    }
}

} // namespace micrortps
} // namespace eprosima

#endif //_MICRORTPS_AGENT_CLIENT_SESSION_STREAM_OUTPUT_STREAM_HPP_
