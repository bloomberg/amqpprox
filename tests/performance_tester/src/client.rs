/*
** Copyright 2022 Bloomberg Finance L.P.
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

use amiquip::{Connection, Exchange, Publish};
use anyhow::Result;

/// Start an AMQP client connecting to address, sending num_messages of message_size.
pub(crate) fn run_sync_client(
    address: String,
    message_size: usize,
    num_messages: usize,
    routing_key: &str,
) -> Result<()> {
    let mut connection = Connection::insecure_open(&address)?;
    let channel = connection.open_channel(None)?;
    let exchange = Exchange::direct(&channel);

    let mut arr = Vec::new();
    arr.resize(message_size, 0);

    let mut count = 0;
    loop {
        if count >= num_messages {
            break;
        }
        exchange.publish(Publish::new(&arr, routing_key))?;

        count += 1;
    }

    connection.close()?;

    Ok(())
}
