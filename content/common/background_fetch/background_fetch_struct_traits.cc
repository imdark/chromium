// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/background_fetch/background_fetch_struct_traits.h"

#include "content/common/service_worker/service_worker.mojom.h"
#include "mojo/public/cpp/bindings/array_data_view.h"
#include "third_party/blink/public/common/manifest/manifest_mojom_traits.h"
#include "third_party/blink/public/mojom/manifest/manifest.mojom.h"

namespace mojo {

// static
bool StructTraits<blink::mojom::BackgroundFetchRegistrationDataView,
                  content::BackgroundFetchRegistration>::
    Read(blink::mojom::BackgroundFetchRegistrationDataView data,
         content::BackgroundFetchRegistration* registration) {
  if (!data.ReadDeveloperId(&registration->developer_id) ||
      !data.ReadUniqueId(&registration->unique_id)) {
    return false;
  }

  registration->upload_total = data.upload_total();
  registration->uploaded = data.uploaded();
  registration->download_total = data.download_total();
  registration->downloaded = data.downloaded();
  registration->result = data.result();
  registration->failure_reason = data.failure_reason();
  return true;
}

// static
bool StructTraits<blink::mojom::BackgroundFetchSettledFetchDataView,
                  content::BackgroundFetchSettledFetch>::
    Read(blink::mojom::BackgroundFetchSettledFetchDataView data,
         content::BackgroundFetchSettledFetch* fetch) {
  return data.ReadRequest(&fetch->request) &&
         data.ReadResponse(&fetch->response);
}

}  // namespace mojo
