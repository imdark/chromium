// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/autofill/local_card_migration_dialog_controller_impl.h"

#include <stddef.h>

#include "base/bind.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/ui/autofill/local_card_migration_dialog.h"
#include "chrome/browser/ui/autofill/local_card_migration_dialog_factory.h"
#include "chrome/browser/ui/autofill/local_card_migration_dialog_state.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "components/autofill/core/browser/autofill_metrics.h"
#include "components/autofill/core/browser/local_card_migration_manager.h"
#include "components/autofill/core/browser/payments/payments_service_url.h"
#include "components/autofill/core/browser/validation.h"
#include "components/autofill/core/common/autofill_clock.h"
#include "components/autofill/core/common/autofill_prefs.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"

namespace autofill {

LocalCardMigrationDialogControllerImpl::LocalCardMigrationDialogControllerImpl(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      pref_service_(
          user_prefs::UserPrefs::Get(web_contents->GetBrowserContext())) {}

LocalCardMigrationDialogControllerImpl::
    ~LocalCardMigrationDialogControllerImpl() {
  if (local_card_migration_dialog_)
    local_card_migration_dialog_->CloseDialog();
}

void LocalCardMigrationDialogControllerImpl::ShowOfferDialog(
    std::unique_ptr<base::DictionaryValue> legal_message,
    const std::vector<MigratableCreditCard>& migratable_credit_cards,
    AutofillClient::LocalCardMigrationCallback start_migrating_cards_callback) {
  if (local_card_migration_dialog_)
    local_card_migration_dialog_->CloseDialog();

  if (!LegalMessageLine::Parse(*legal_message, &legal_message_lines_,
                               /*escape_apostrophes=*/true)) {
    AutofillMetrics::LogLocalCardMigrationDialogOfferMetric(
        AutofillMetrics::
            LOCAL_CARD_MIGRATION_DIALOG_NOT_SHOWN_INVALID_LEGAL_MESSAGE);
    return;
  }

  view_state_ = LocalCardMigrationDialogState::kOffered;
  // Need to create the icon first otherwise the dialog will not be shown.
  UpdateIcon();
  local_card_migration_dialog_ =
      CreateLocalCardMigrationDialogView(this, web_contents());
  start_migrating_cards_callback_ = std::move(start_migrating_cards_callback);
  migratable_credit_cards_ = migratable_credit_cards;
  local_card_migration_dialog_->ShowDialog();
  UpdateIcon();
  dialog_is_visible_duration_timer_ = base::ElapsedTimer();

  AutofillMetrics::LogLocalCardMigrationDialogOfferMetric(
      AutofillMetrics::LOCAL_CARD_MIGRATION_DIALOG_SHOWN);
}

void LocalCardMigrationDialogControllerImpl::UpdateCreditCardIcon(
    const base::string16& tip_message,
    const std::vector<MigratableCreditCard>& migratable_credit_cards,
    AutofillClient::MigrationDeleteCardCallback delete_local_card_callback) {
  if (local_card_migration_dialog_)
    local_card_migration_dialog_->CloseDialog();

  migratable_credit_cards_ = migratable_credit_cards;
  tip_message_ = tip_message;
  delete_local_card_callback_ = delete_local_card_callback;

  view_state_ = LocalCardMigrationDialogState::kFinished;
  for (const auto& cc : migratable_credit_cards) {
    if (cc.migration_status() ==
        MigratableCreditCard::MigrationStatus::FAILURE_ON_UPLOAD) {
      view_state_ = LocalCardMigrationDialogState::kActionRequired;
      break;
    }
  }
  UpdateIcon();
}

void LocalCardMigrationDialogControllerImpl::ShowFeedbackDialog() {
  local_card_migration_dialog_ =
      CreateLocalCardMigrationDialogView(this, web_contents());
  local_card_migration_dialog_->ShowDialog();
  UpdateIcon();
}

void LocalCardMigrationDialogControllerImpl::ShowErrorDialog() {
  local_card_migration_dialog_ =
      CreateLocalCardMigrationErrorDialogView(this, web_contents());
  UpdateIcon();
  local_card_migration_dialog_->ShowDialog();
}

void LocalCardMigrationDialogControllerImpl::AddObserver(
    LocalCardMigrationControllerObserver* observer) {
  observer_list_.AddObserver(observer);
}

LocalCardMigrationDialogState
LocalCardMigrationDialogControllerImpl::GetViewState() const {
  return view_state_;
}

const std::vector<MigratableCreditCard>&
LocalCardMigrationDialogControllerImpl::GetCardList() const {
  return migratable_credit_cards_;
}

const LegalMessageLines&
LocalCardMigrationDialogControllerImpl::GetLegalMessageLines() const {
  return legal_message_lines_;
}

const base::string16& LocalCardMigrationDialogControllerImpl::GetTipMessage()
    const {
  return tip_message_;
}

void LocalCardMigrationDialogControllerImpl::OnSaveButtonClicked(
    const std::vector<std::string>& selected_cards_guids) {
  AutofillMetrics::LogLocalCardMigrationDialogUserInteractionMetric(
      dialog_is_visible_duration_timer_.Elapsed(), selected_cards_guids.size(),
      migratable_credit_cards_.size(),
      AutofillMetrics::LOCAL_CARD_MIGRATION_DIALOG_CLOSED_SAVE_BUTTON_CLICKED);

  std::move(start_migrating_cards_callback_).Run(selected_cards_guids);
  NotifyMigrationStarted();
}

void LocalCardMigrationDialogControllerImpl::OnCancelButtonClicked() {
  AutofillMetrics::LogLocalCardMigrationDialogUserInteractionMetric(
      dialog_is_visible_duration_timer_.Elapsed(), 0,
      migratable_credit_cards_.size(),
      AutofillMetrics::
          LOCAL_CARD_MIGRATION_DIALOG_CLOSED_CANCEL_BUTTON_CLICKED);

  prefs::SetLocalCardMigrationPromptPreviouslyCancelled(pref_service_, true);

  start_migrating_cards_callback_.Reset();
  NotifyMigrationNoLongerAvailable();
}

void LocalCardMigrationDialogControllerImpl::OnDoneButtonClicked() {
  NotifyMigrationNoLongerAvailable();
}

void LocalCardMigrationDialogControllerImpl::OnViewCardsButtonClicked() {
  // TODO(crbug.com/867194): Add metrics.
  constexpr int kPaymentsProfileUserIndex = 0;
  OpenUrl(payments::GetManageInstrumentsUrl(kPaymentsProfileUserIndex));
  NotifyMigrationNoLongerAvailable();
}

void LocalCardMigrationDialogControllerImpl::OnLegalMessageLinkClicked(
    const GURL& url) {
  OpenUrl(url);
  AutofillMetrics::LogLocalCardMigrationDialogUserInteractionMetric(
      dialog_is_visible_duration_timer_.Elapsed(), 0,
      migratable_credit_cards_.size(),
      AutofillMetrics::LOCAL_CARD_MIGRATION_DIALOG_LEGAL_MESSAGE_CLICKED);
}

void LocalCardMigrationDialogControllerImpl::DeleteCard(
    const std::string& deleted_card_guid) {
  DCHECK(delete_local_card_callback_);
  delete_local_card_callback_.Run(deleted_card_guid);

  migratable_credit_cards_.erase(
      std::remove_if(migratable_credit_cards_.begin(),
                     migratable_credit_cards_.end(),
                     [&](const auto& card) {
                       return card.credit_card().guid() == deleted_card_guid;
                     }),
      migratable_credit_cards_.end());

  if (!HasFailedCard()) {
    view_state_ = LocalCardMigrationDialogState::kFinished;
    delete_local_card_callback_.Reset();
  }
}

void LocalCardMigrationDialogControllerImpl::OnDialogClosed() {
  if (local_card_migration_dialog_)
    local_card_migration_dialog_ = nullptr;

  UpdateIcon();
}

bool LocalCardMigrationDialogControllerImpl::AllCardsInvalid() const {
  // For kOffered state, the migration status of all cards are UNKNOWN,
  // so this function will return true as well. Need an early exit to avoid
  // it.
  if (view_state_ == LocalCardMigrationDialogState::kOffered)
    return false;

  return std::find_if(
             migratable_credit_cards_.begin(), migratable_credit_cards_.end(),
             [](const auto& card) {
               return card.migration_status() ==
                      MigratableCreditCard::MigrationStatus::SUCCESS_ON_UPLOAD;
             }) == migratable_credit_cards_.end();
}

LocalCardMigrationDialog*
LocalCardMigrationDialogControllerImpl::local_card_migration_dialog_view()
    const {
  return local_card_migration_dialog_;
}

void LocalCardMigrationDialogControllerImpl::OpenUrl(const GURL& url) {
  web_contents()->OpenURL(content::OpenURLParams(
      url, content::Referrer(), WindowOpenDisposition::NEW_POPUP,
      ui::PAGE_TRANSITION_LINK, false));
}

void LocalCardMigrationDialogControllerImpl::UpdateIcon() {
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents());
  DCHECK(browser);
  LocationBar* location_bar = browser->window()->GetLocationBar();
  DCHECK(location_bar);
  location_bar->UpdateLocalCardMigrationIcon();
}

bool LocalCardMigrationDialogControllerImpl::HasFailedCard() const {
  return std::find_if(
             migratable_credit_cards_.begin(), migratable_credit_cards_.end(),
             [](const auto& card) {
               return card.migration_status() ==
                      MigratableCreditCard::MigrationStatus::FAILURE_ON_UPLOAD;
             }) != migratable_credit_cards_.end();
}

void LocalCardMigrationDialogControllerImpl::
    NotifyMigrationNoLongerAvailable() {
  for (LocalCardMigrationControllerObserver& observer : observer_list_) {
    observer.OnMigrationNoLongerAvailable();
  }
}

void LocalCardMigrationDialogControllerImpl::NotifyMigrationStarted() {
  for (LocalCardMigrationControllerObserver& observer : observer_list_) {
    observer.OnMigrationStarted();
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(LocalCardMigrationDialogControllerImpl)

}  // namespace autofill
