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

use amq_protocol::frame::AMQPFrame;
use amq_protocol::frame::GenError;
use amq_protocol::frame::WriteContext;
use amq_protocol::protocol::channel;
use amq_protocol::protocol::confirm;
use amq_protocol::protocol::connection;
use amq_protocol::protocol::exchange;
use amq_protocol::protocol::queue;
use amq_protocol::protocol::AMQPClass;
use amq_protocol::types::AMQPValue;
use anyhow::bail;
use anyhow::Result;
use bytes::BytesMut;
use futures::SinkExt;
use futures::StreamExt;
use rustls_pemfile::{certs, pkcs8_private_keys};
use std::fs::File;
use std::io;
use std::io::BufReader;
use std::io::Cursor;
use std::net::SocketAddr;
use std::path::Path;
use std::sync::Arc;
use tokio::io::AsyncReadExt;
use tokio::net::TcpListener;
use tokio::net::TcpSocket;
use tokio_rustls::rustls::{self, Certificate, PrivateKey};
use tokio_rustls::TlsAcceptor;
use AMQPFrame::{Heartbeat, Method};

fn load_certs(path: &Path) -> io::Result<Vec<Certificate>> {
    certs(&mut BufReader::new(File::open(path)?))
        .map_err(|_| io::Error::new(io::ErrorKind::InvalidInput, "invalid cert"))
        .map(|mut certs| certs.drain(..).map(Certificate).collect())
}

fn load_keys(path: &Path) -> io::Result<Vec<PrivateKey>> {
    pkcs8_private_keys(&mut BufReader::new(File::open(path)?))
        .map_err(|_| io::Error::new(io::ErrorKind::InvalidInput, "invalid key"))
        .map(|mut keys| keys.drain(..).map(PrivateKey).collect())
}

#[derive(thiserror::Error, Debug)]
enum AMQPCodecError {
    #[error("Underlying Error: {0}")]
    Underlying(String),

    #[error("Generate error")]
    GenError(#[from] GenError),

    #[error(transparent)]
    Other(#[from] anyhow::Error),
}

impl From<std::io::Error> for AMQPCodecError {
    fn from(error: std::io::Error) -> AMQPCodecError {
        AMQPCodecError::Underlying(error.to_string())
    }
}

struct AMQPCodec {}

impl tokio_util::codec::Decoder for AMQPCodec {
    type Item = AMQPFrame;
    type Error = AMQPCodecError;
    fn decode(&mut self, src: &mut BytesMut) -> Result<Option<Self::Item>, Self::Error> {
        log::trace!("Attempt decode from: {} bytes", src.len());
        let (consumed, res) = match amq_protocol::frame::parse_frame(src) {
            Ok((consumed, frame)) => (src.len() - consumed.len(), Ok(Some(frame))),
            Err(e) => {
                if e.is_incomplete() {
                    (0, Ok(None))
                } else {
                    (
                        0,
                        Err(AMQPCodecError::Underlying(format!(
                            "Parse error for frame: {:?}",
                            e
                        ))),
                    )
                }
            }
        };

        log::trace!("Consumed: {}, Res: {:?}", consumed, res);
        let _ = src.split_to(consumed);
        res
    }
}

impl tokio_util::codec::Encoder<AMQPFrame> for AMQPCodec {
    type Error = AMQPCodecError;
    fn encode(&mut self, item: AMQPFrame, dst: &mut BytesMut) -> Result<(), Self::Error> {
        loop {
            let res = amq_protocol::frame::gen_frame(&item)(WriteContext::from(Cursor::new(
                dst.as_mut(),
            )));
            match res {
                Ok(wc) => {
                    let (_writer, position) = wc.into_inner();
                    dst.resize(position as usize, 0);
                    return Ok(());
                }
                Err(amq_protocol::frame::GenError::BufferTooSmall(sz)) => {
                    let capacity = dst.capacity();
                    dst.resize(capacity + sz, 0);
                }
                Err(e) => {
                    return Err(e.into());
                }
            }
        }
    }
}

impl AMQPCodec {
    fn new() -> Self {
        Self {}
    }
}

pub async fn process_connection<
    Stream: tokio::io::AsyncRead + std::marker::Unpin + tokio::io::AsyncWrite,
>(
    mut socket: Stream,
) -> Result<()> {
    let mut buf: [u8; 8] = [0; 8];
    socket.read_exact(&mut buf).await?; // We ignore if it's correct

    log::debug!("Protocol header received");

    let codec = AMQPCodec::new();
    let mut framed = tokio_util::codec::Framed::new(socket, codec);

    let mut server_props = amq_protocol::types::FieldTable::default();
    server_props.insert(
        String::from("product").into(),
        AMQPValue::LongString(String::from("amqpprox_perf_test").into()),
    );

    let start_method = connection::AMQPMethod::Start(connection::Start {
        version_major: 0,
        version_minor: 9,
        mechanisms: "PLAIN".to_string().into(),
        locales: "en_US".to_string().into(),
        server_properties: server_props,
    });
    let start = Method(0, AMQPClass::Connection(start_method));
    framed.send(start).await?;

    let frame = framed.next().await;
    if let Some(Ok(Method(0, AMQPClass::Connection(connection::AMQPMethod::StartOk(frame))))) =
        frame
    {
        log::debug!("Should be start-ok: {:?}", frame);
        let tune_method = connection::AMQPMethod::Tune(connection::Tune {
            channel_max: 2047,
            frame_max: 131072,
            heartbeat: 60,
        });
        let tune = Method(0, AMQPClass::Connection(tune_method));
        framed.send(tune).await?;
    } else {
        bail!("Invalid protocol, received: {:?}", frame);
    }

    let frame = framed.next().await;
    if let Some(Ok(Method(0, AMQPClass::Connection(connection::AMQPMethod::TuneOk(frame))))) = frame
    {
        log::debug!("Should be tune-ok: {:?}", frame);
    } else {
        bail!("Invalid protocol, received: {:?}", frame);
    }

    let frame = framed.next().await;
    if let Some(Ok(Method(0, AMQPClass::Connection(connection::AMQPMethod::Open(frame))))) = frame {
        log::debug!("Should be open: {:?}", frame);
        let openok_method = connection::AMQPMethod::OpenOk(connection::OpenOk {});
        let openok = Method(0, AMQPClass::Connection(openok_method));
        framed.send(openok).await?;
    } else {
        bail!("Invalid protocol, received: {:?}", frame);
    }

    log::info!("Handshake complete");
    while let Some(frame) = framed.next().await {
        log::trace!("Received: {:?}", &frame);
        match frame {
            Ok(Method(channel, AMQPClass::Channel(channel::AMQPMethod::Open(channelmsg)))) => {
                log::debug!("Set up channel: {} {:?}", channel, channelmsg);
                let channelok_method = channel::AMQPMethod::OpenOk(channel::OpenOk {});
                let channelok = Method(channel, AMQPClass::Channel(channelok_method));
                framed.send(channelok).await?;
            }
            Ok(Method(channel, AMQPClass::Channel(channel::AMQPMethod::Close(_channelmsg)))) => {
                log::debug!("Received channel close");
                let closeok = Method(
                    channel,
                    AMQPClass::Channel(channel::AMQPMethod::CloseOk(channel::CloseOk {})),
                );
                framed.send(closeok).await?;
            }
            Ok(Method(0, AMQPClass::Connection(connection::AMQPMethod::Close(closemsg)))) => {
                log::info!("Closing connection requested: {:?}", closemsg);
                let closeok_method = connection::AMQPMethod::CloseOk(connection::CloseOk {});
                let closeok = Method(0, AMQPClass::Connection(closeok_method));
                framed.send(closeok).await?;
                log::info!("Closing connection requested: {:?}. Sent CloseOk", closemsg);
                return Ok(());
            }
            Ok(Method(channel, AMQPClass::Exchange(exchange::AMQPMethod::Declare(declaremsg)))) => {
                log::debug!("Exchange Declare: {} {:?}", channel, declaremsg);
                let declareok_method = exchange::AMQPMethod::DeclareOk(exchange::DeclareOk {});
                let deckareok = Method(channel, AMQPClass::Exchange(declareok_method));
                framed.send(deckareok).await?;
            }
            Ok(Method(channel, AMQPClass::Queue(queue::AMQPMethod::Declare(declaremsg)))) => {
                log::debug!("Queue Declare: {} {:?}", channel, declaremsg);
                let declareok_method = queue::AMQPMethod::DeclareOk(queue::DeclareOk {
                    consumer_count: 0,
                    message_count: 0,
                    queue: amq_protocol::types::ShortString::from(declaremsg.queue),
                });
                let declareok = Method(channel, AMQPClass::Queue(declareok_method));
                framed.send(declareok).await?;
            }
            Ok(Method(channel, AMQPClass::Exchange(exchange::AMQPMethod::Bind(bindmsg)))) => {
                log::debug!("Exchange Bind: {} {:?}", channel, bindmsg);
                let bindok_method = exchange::AMQPMethod::BindOk(exchange::BindOk {});
                let bindok = Method(channel, AMQPClass::Exchange(bindok_method));
                framed.send(bindok).await?;
            }
            Ok(Method(channel, AMQPClass::Queue(queue::AMQPMethod::Bind(bindmsg)))) => {
                log::debug!("Queue Bind: {} {:?}", channel, bindmsg);
                let bindok_method = queue::AMQPMethod::BindOk(queue::BindOk {});
                let bindok = Method(channel, AMQPClass::Queue(bindok_method));
                framed.send(bindok).await?;
            }
            Ok(Method(channel, AMQPClass::Confirm(confirm::AMQPMethod::Select(select)))) => {
                log::debug!("Confirm Select: {} {:?}", channel, select);
                let selectok_method = confirm::AMQPMethod::SelectOk(confirm::SelectOk {});
                let selectok = Method(channel, AMQPClass::Confirm(selectok_method));
                framed.send(selectok).await?;
            }
            Ok(Heartbeat(channel)) => {
                log::debug!("Heartbeat received: {channel}");
                let reply = Heartbeat(channel);
                framed.send(reply).await?;
            }
            _ => {}
        }
    }
    Ok(())
}

pub fn create_tls_acceptor(cert_chain: &Path, key: &Path) -> Result<TlsAcceptor> {
    let certs = load_certs(cert_chain)?;
    let mut keys = load_keys(key)?;

    log::info!("{:?} {:?}", certs, keys);

    let config = rustls::ServerConfig::builder()
        .with_safe_defaults()
        .with_no_client_auth()
        .with_single_cert(certs, keys.remove(0))
        .map_err(|err| io::Error::new(io::ErrorKind::InvalidInput, err))?;

    Ok(TlsAcceptor::from(Arc::new(config)))
}

pub async fn run_tls_server(address: SocketAddr, acceptor: TlsAcceptor) -> Result<()> {
    log::info!("Listening on {:?}", &address);

    let socket = TcpSocket::new_v4()?;
    socket.set_reuseaddr(true)?;
    socket.bind(address)?;

    let listener = socket.listen(1024)?;

    loop {
        let (socket, peer) = listener.accept().await?;
        let acceptor = acceptor.clone();
        log::debug!("Connection from {}", peer);
        tokio::spawn(async move {
            let stream = acceptor.accept(socket).await?;

            process_connection(stream).await
        });
    }
}

pub async fn run_server(address: SocketAddr) -> Result<()> {
    log::info!("Listening on {:?}", &address);
    let listener = TcpListener::bind(address).await?;

    loop {
        let (socket, peer) = listener.accept().await?;
        log::debug!("Connection from {}", peer);
        tokio::spawn(async move { process_connection(socket).await });
    }
}
