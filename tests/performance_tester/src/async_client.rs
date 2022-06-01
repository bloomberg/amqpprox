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

use lapin::{options::*, BasicProperties, Connection, ConnectionProperties, Result};

use tokio_executor_trait;

use rand::Rng;

/// Start an AMQP client connecting to address, sending num_messages of message_size.
pub(crate) async fn run_async_client(
    address: String,
    message_size: usize,
    num_messages: usize,
    routing_key: &str,
    publish_wait_ms: u64,
    delay_start_ms: u64,
) -> Result<()> {
    let start_delay = rand::thread_rng().gen_range(0..=delay_start_ms);

    tokio::time::sleep(tokio::time::Duration::from_millis(start_delay)).await;

    let options = ConnectionProperties::default()
        .with_executor(tokio_executor_trait::Tokio::current())
        .with_reactor(tokio_reactor_trait::Tokio);

    let conn = Connection::connect(&address, options).await?;

    let channel = conn.create_channel().await?;

    let mut arr = Vec::new();
    arr.resize(message_size, 0);

    let mut count = 0;
    loop {
        if count >= num_messages {
            break;
        }

        let _confirm = channel
            .basic_publish(
                "",
                routing_key,
                BasicPublishOptions::default(),
                &arr,
                BasicProperties::default(),
            )
            .await?
            .await?;

        tokio::time::sleep(tokio::time::Duration::from_millis(publish_wait_ms)).await;

        count += 1;
    }

    let _ = conn.close(200, "Closing").await;

    Ok(())
}
