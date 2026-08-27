/*
** Copyright 2026 Bloomberg Finance L.P.
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

#include <amqpprox_tlsutil.h>

#include <gtest/gtest.h>

#include <boost/asio/ssl.hpp>

#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

using namespace Bloomberg;
using namespace amqpprox;

namespace {

class StoreContextGuard {
    X509_STORE_CTX *d_ctx;

  public:
    StoreContextGuard()
    : d_ctx(X509_STORE_CTX_new())
    {
    }

    ~StoreContextGuard()
    {
        if (d_ctx) {
            X509_STORE_CTX_free(d_ctx);
        }
    }

    StoreContextGuard(const StoreContextGuard &)            = delete;
    StoreContextGuard &operator=(const StoreContextGuard &) = delete;

    X509_STORE_CTX *get() { return d_ctx; }
};

class CertificateGuard {
    X509 *d_cert;

  public:
    CertificateGuard()
    : d_cert(X509_new())
    {
    }

    ~CertificateGuard()
    {
        if (d_cert) {
            X509_free(d_cert);
        }
    }

    CertificateGuard(const CertificateGuard &)            = delete;
    CertificateGuard &operator=(const CertificateGuard &) = delete;

    X509 *get() { return d_cert; }
};

}

TEST(TlsUtil, LogCertVerificationFailureNullCurrentCert)
{
    StoreContextGuard storeCtx;
    ASSERT_NE(storeCtx.get(), nullptr);
    ASSERT_EQ(X509_STORE_CTX_init(storeCtx.get(), nullptr, nullptr, nullptr),
              1);
    ASSERT_EQ(X509_STORE_CTX_get_current_cert(storeCtx.get()), nullptr);

    boost::asio::ssl::verify_context ctx(storeCtx.get());

    EXPECT_FALSE(TlsUtil::logCertVerificationFailure(false, ctx));
}

TEST(TlsUtil, LogCertVerificationFailureNullStoreContext)
{
    boost::asio::ssl::verify_context ctx(nullptr);

    EXPECT_FALSE(TlsUtil::logCertVerificationFailure(false, ctx));
}

TEST(TlsUtil, LogCertVerificationFailureWithSubject)
{
    StoreContextGuard storeCtx;
    ASSERT_NE(storeCtx.get(), nullptr);
    ASSERT_EQ(X509_STORE_CTX_init(storeCtx.get(), nullptr, nullptr, nullptr),
              1);

    CertificateGuard cert;
    ASSERT_NE(cert.get(), nullptr);
    ASSERT_NE(X509_get_subject_name(cert.get()), nullptr);

    X509_STORE_CTX_set_current_cert(storeCtx.get(), cert.get());

    boost::asio::ssl::verify_context ctx(storeCtx.get());

    EXPECT_FALSE(TlsUtil::logCertVerificationFailure(false, ctx));
}

TEST(TlsUtil, LogCertVerificationPreverifiedPassesThrough)
{
    StoreContextGuard storeCtx;
    ASSERT_NE(storeCtx.get(), nullptr);
    ASSERT_EQ(X509_STORE_CTX_init(storeCtx.get(), nullptr, nullptr, nullptr),
              1);

    boost::asio::ssl::verify_context ctx(storeCtx.get());

    EXPECT_TRUE(TlsUtil::logCertVerificationFailure(true, ctx));
}
