# Lobby updates

## Summary
- Added a confirmation dialog when selecting a friend in the lobby mini friend list.
- Dialog is modal: LEFT/RIGHT/SELECT/EXIT are routed to the dialog while visible.
- Dialog is drawn on top of the lobby screen.
- Confirm action sends game invite via API, Cancel closes the dialog.
- Added API method `inviteToSession()` to send invite to existing session.

## Files updated
- [src/game_lobby_screen.h](src/game_lobby_screen.h)
- [src/game_lobby_screen.cpp](src/game_lobby_screen.cpp)
- [src/api_client.h](src/api_client.h)
- [src/api_client.cpp](src/api_client.cpp)
- [src/social_screen.cpp](src/social_screen.cpp)

## Details
### New members (GameLobbyScreen)
- `confirmationDialog`
- `pendingInviteFriendIndex`
- `pendingInviteFriendName`
- `currentSessionId`, `hostUserId`, `serverHost`, `serverPort` (for API calls)

### New methods (GameLobbyScreen)
- `showInviteConfirmation()`
- `doConfirmInvite()` - sends API request to invite friend
- `doCancelInvite()`
- `onConfirmInvite()` / `onCancelInvite()` (static callbacks)
- `setSessionInfo()` - set session and server info for invites

### New API method (ApiClient)
- `inviteToSession(sessionId, hostUserId, participantIds[], count, serverHost, port)` - POST `/api/games/{session_id}/invite`

### Behavior changes
- `handleSelect()` (left panel): shows confirm dialog instead of immediately inviting.
- `handleKeyPress()`: routes dialog input while visible.
- `draw()`: draws dialog after lobby UI.
- `MiniFriend` struct: added `userId` field for API calls.
- `SocialScreen`: passes `userId` when converting friends to `MiniFriend[]`, and calls `setSessionInfo()` after creating session.
