// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_WEB_DATABASE_HOST_IMPL_H_
#define CONTENT_BROWSER_RENDERER_HOST_WEB_DATABASE_HOST_IMPL_H_

#include <string>

#include "base/callback_forward.h"
#include "base/strings/string16.h"
#include "build/build_config.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "storage/browser/database/database_tracker.h"
#include "third_party/blink/public/platform/modules/webdatabase/web_database.mojom.h"

namespace url {
class Origin;
}  // namespace url

namespace content {

class CONTENT_EXPORT WebDatabaseHostImpl
    : public blink::mojom::WebDatabaseHost,
      public storage::DatabaseTracker::Observer {
 public:
  WebDatabaseHostImpl(int process_id,
                      scoped_refptr<storage::DatabaseTracker> db_tracker);
  ~WebDatabaseHostImpl() override;

  static void Create(int process_id,
                     scoped_refptr<storage::DatabaseTracker> db_tracker,
                     blink::mojom::WebDatabaseHostRequest request);

 private:
  FRIEND_TEST_ALL_PREFIXES(WebDatabaseHostImplTest, BadMessagesUnauthorized);
  FRIEND_TEST_ALL_PREFIXES(WebDatabaseHostImplTest, BadMessagesInvalid);

  // blink::mojom::WebDatabaseHost:
  void OpenFile(const base::string16& vfs_file_name,
                int32_t desired_flags,
                OpenFileCallback callback) override;

  void DeleteFile(const base::string16& vfs_file_name,
                  bool sync_dir,
                  DeleteFileCallback callback) override;

  void GetFileAttributes(const base::string16& vfs_file_name,
                         GetFileAttributesCallback callback) override;

  void GetFileSize(const base::string16& vfs_file_name,
                   GetFileSizeCallback callback) override;

  void SetFileSize(const base::string16& vfs_file_name,
                   int64_t expected_size,
                   SetFileSizeCallback callback) override;

  void GetSpaceAvailable(const url::Origin& origin,
                         GetSpaceAvailableCallback callback) override;

  void Opened(const url::Origin& origin,
              const base::string16& database_name,
              const base::string16& database_description,
              int64_t estimated_size) override;

  void Modified(const url::Origin& origin,
                const base::string16& database_name) override;

  void Closed(const url::Origin& origin,
              const base::string16& database_name) override;

  void HandleSqliteError(const url::Origin& origin,
                         const base::string16& database_name,
                         int32_t error) override;

  // DatabaseTracker::Observer callbacks (tracker sequence)
  void OnDatabaseSizeChanged(const std::string& origin_identifier,
                             const base::string16& database_name,
                             int64_t database_size) override;
  void OnDatabaseScheduledForDeletion(
      const std::string& origin_identifier,
      const base::string16& database_name) override;

  void DatabaseDeleteFile(const base::string16& vfs_file_name,
                          bool sync_dir,
                          DeleteFileCallback callback,
                          int reschedule_count);

  // Helper function to get the mojo interface for the WebDatabase on the
  // render process. Creates the WebDatabase connection if it does not already
  // exist.
  blink::mojom::WebDatabase& GetWebDatabase();

  // blink::mojom::WebDatabaseHost methods called after ValidateOrigin()
  // successfully validates the origin.
  void OpenFileValidated(const base::string16& vfs_file_name,
                         int32_t desired_flags,
                         OpenFileCallback callback);

  void GetFileAttributesValidated(const base::string16& vfs_file_name,
                                  GetFileAttributesCallback callback);

  void GetFileSizeValidated(const base::string16& vfs_file_name,
                            GetFileSizeCallback callback);

  void SetFileSizeValidated(const base::string16& vfs_file_name,
                            int64_t expected_size,
                            SetFileSizeCallback callback);

  void GetSpaceAvailableValidated(const url::Origin& origin,
                                  GetSpaceAvailableCallback callback);

  void OpenedValidated(const url::Origin& origin,
                       const base::string16& database_name,
                       const base::string16& database_description,
                       int64_t estimated_size);

  void ModifiedValidated(const url::Origin& origin,
                         const base::string16& database_name);

  void ClosedValidated(const url::Origin& origin,
                       const base::string16& database_name);

  void HandleSqliteErrorValidated(const url::Origin& origin,
                                  const base::string16& database_name,
                                  int32_t error);

  // Asynchronously calls |callback| but only if |process_id_| has permission to
  // access the passed |origin|.  Must be called from within the context of a
  // mojo call. Invalid calls will report a bad message, which will terminate
  // the calling process.
  void ValidateOrigin(const url::Origin& origin, base::OnceClosure callback);

  // As above, but for calls where the origin is embedded in a VFS filename.
  // Empty filenames signalling a temp file are permitted.
  void ValidateOrigin(const base::string16& vfs_file_name,
                      base::OnceClosure callback);
  // Our render process host ID, used to bind to the correct render process.
  const int process_id_;

  // True if and only if this instance was added as an observer
  // to DatabaseTracker.
  bool observer_added_;

  // Keeps track of all DB connections opened by this renderer
  storage::DatabaseConnections database_connections_;

  // Interface to the render process WebDatabase.
  blink::mojom::WebDatabasePtr database_provider_;

  // The database tracker for the current browser context.
  const scoped_refptr<storage::DatabaseTracker> db_tracker_;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<WebDatabaseHostImpl> weak_ptr_factory_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_WEB_DATABASE_HOST_IMPL_H_
