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

use anyhow::Result;
use clap::Parser;
use std::net::SocketAddr;
use std::path::PathBuf;
use std::time::Duration;
use std::time::Instant;
use tokio::runtime::Builder;

mod client;
mod server;

#[derive(Debug, Parser, Clone)]
struct PerfTesterOpts {
    #[clap(long, default_value = "amqp://localhost:5672/")]
    address: String,

    #[clap(
        long,
        default_value_t = 10,
        help = "Number of total AMQP clients to run"
    )]
    clients: usize,

    #[clap(long, default_value_t = 100)]
    message_size: usize,

    #[clap(long, default_value_t = 10)]
    num_messages: usize,

    #[clap(
        long,
        default_value_t = 50,
        help = "Max AMQP clients which can run in parallel"
    )]
    max_threads: usize,

    #[clap(long, help = "IP Address/port for the dummy AMQP server to listen on")]
    listen_address: SocketAddr,

    #[clap(long, help = "TLS cer used by the dummy AMQP server")]
    listen_cert: Option<PathBuf>,

    #[clap(
        long,
        help = "TLS key used by the dummy AMQP server. Must be the appropriate key for the provided cert"
    )]
    listen_key: Option<PathBuf>,

    #[clap(
        long,
        default_value = "routing-key",
        help = "Routing key passed for sent messages"
    )]
    routing_key: String,
}

fn main() -> Result<()> {
    env_logger::init();
    let opts = PerfTesterOpts::parse();

    let start = Instant::now();

    let mut success = 0;

    {
        let runtime = Builder::new_multi_thread()
            .enable_all()
            .max_blocking_threads(opts.max_threads)
            .build()
            .unwrap();

        let opts = opts.clone();
        runtime.block_on(async {
            println!("Starting performance test of amqpprox");

            let address = opts.listen_address;
            let _server = if let (Some(listen_cert), Some(listen_key)) =
                (opts.listen_cert, opts.listen_key)
            {
                println!("Starting TLS dummy amqp server");
                let acceptor = server::create_tls_acceptor(&listen_cert, &listen_key).unwrap();
                tokio::spawn(async move { server::run_tls_server(address, acceptor).await })
            } else {
                println!("Starting non-TLS dummy amqp server");
                tokio::spawn(async move { server::run_server(address).await })
            };

            wait_for_addr(opts.listen_address, Duration::from_millis(10000))
                .await
                .unwrap();

            let mut handles = Vec::new();
            for _ in 0..opts.clients {
                let address = opts.address.clone();
                let message_size = opts.message_size;
                let num_messages = opts.num_messages;
                let routing_key = opts.routing_key.clone();

                let handle = tokio::task::spawn_blocking(move || {
                    crate::client::run_sync_client(
                        address,
                        message_size,
                        num_messages,
                        &routing_key,
                    )
                });
                handles.push(handle);
            }

            for handle in handles {
                match handle.await.unwrap() {
                    Ok(_) => success += 1,
                    Err(err) => log::error!("Client failed: {:?}", err),
                }
            }
        });
    }

    if success != opts.clients {
        println!("{} clients were not fully successful. Check the logs to see if this will impact perf results", opts.clients - success);
    }

    let duration = start.elapsed();
    let total_bytes = opts.clients * opts.num_messages * opts.message_size;
    println!(
        "{} clients and {}KiB in {}seconds",
        opts.clients,
        total_bytes / 1024,
        duration.as_secs_f64()
    );

    let clients_per_sec = opts.clients as f64 / duration.as_secs_f64();
    let bytes_per_sec = total_bytes as f64 / duration.as_secs_f64();

    println!(
        "{} connections/second, {} MiB/second",
        clients_per_sec,
        bytes_per_sec / 1024f64 / 1024f64
    );

    Ok(())
}

async fn wait_for_addr(addr: SocketAddr, timeout_total: Duration) -> Result<()> {
    let iterations = 10;
    let timeout_step = timeout_total / iterations;

    let mut iteration = 1;

    loop {
        let start = Instant::now();

        match try_addr(addr, timeout_step).await {
            Ok(()) => return Ok(()),
            Err(err) => {
                println!("Waiting for dummy server to start: {:?}", err);
                if iteration == iterations {
                    return Err(err);
                }
            }
        }

        let target = start
            .checked_add(timeout_step)
            .ok_or(anyhow::anyhow!("Timeout add overflowed"))?;
        if let Some(sleep_time) = target.checked_duration_since(Instant::now()) {
            tokio::time::sleep(sleep_time).await;
        }

        iteration += 1;
    }
}

async fn try_addr(addr: SocketAddr, timeout: Duration) -> Result<()> {
    let connect = tokio::net::TcpStream::connect(addr);

    let _: tokio::net::TcpStream = tokio::time::timeout(timeout, connect).await??;

    Ok(())
}
