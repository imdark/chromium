// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_LOADER_WEBURLRESPONSE_EXTRADATA_IMPL_H_
#define CONTENT_RENDERER_LOADER_WEBURLRESPONSE_EXTRADATA_IMPL_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/common/content_export.h"
#include "net/http/http_response_info.h"
#include "net/nqe/effective_connection_type.h"
#include "third_party/blink/public/platform/web_url_response.h"

namespace content {

class CONTENT_EXPORT WebURLResponseExtraDataImpl
    : public blink::WebURLResponse::ExtraData {
 public:
  WebURLResponseExtraDataImpl();
  ~WebURLResponseExtraDataImpl() override;

  /// Flag whether this request was loaded via the SPDY protocol or not.
  // SPDY is an experimental web protocol, see http://dev.chromium.org/spdy
  bool was_fetched_via_spdy() const {
    return was_fetched_via_spdy_;
  }
  void set_was_fetched_via_spdy(bool was_fetched_via_spdy) {
    was_fetched_via_spdy_ = was_fetched_via_spdy;
  }

  // Flag whether this request was loaded after the
  // TLS/Next-Protocol-Negotiation was used.
  // This is related to SPDY.
  bool was_alpn_negotiated() const { return was_alpn_negotiated_; }
  void set_was_alpn_negotiated(bool was_alpn_negotiated) {
    was_alpn_negotiated_ = was_alpn_negotiated;
  }

  // Flag whether this request was made when "Alternate-Protocol: xxx"
  // is present in server's response.
  bool was_alternate_protocol_available() const {
    return was_alternate_protocol_available_;
  }
  void set_was_alternate_protocol_available(
      bool was_alternate_protocol_available) {
    was_alternate_protocol_available_ = was_alternate_protocol_available;
  }

  net::EffectiveConnectionType effective_connection_type() const {
    return effective_connection_type_;
  }
  void set_effective_connection_type(
      net::EffectiveConnectionType effective_connection_type) {
    effective_connection_type_ = effective_connection_type;
  }

 private:
  bool was_fetched_via_spdy_;
  bool was_alpn_negotiated_;
  bool was_alternate_protocol_available_;
  net::EffectiveConnectionType effective_connection_type_;

  DISALLOW_COPY_AND_ASSIGN(WebURLResponseExtraDataImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_LOADER_WEBURLRESPONSE_EXTRADATA_IMPL_H_
