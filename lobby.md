# Lobby updates

## Summary
- Added a confirmation dialog when selecting a friend in the lobby mini friend list.
- Dialog is modal: LEFT/RIGHT/SELECT/EXIT are routed to the dialog while visible.
- Dialog is drawn on top of the lobby screen.
- Confirm/Cancel actions close the dialog and return to lobby.

## Files updated
- [src/game_lobby_screen.h](src/game_lobby_screen.h)
- [src/game_lobby_screen.cpp](src/game_lobby_screen.cpp)

## Details
### New members (GameLobbyScreen)
- `confirmationDialog`
- `pendingInviteFriendIndex`
- `pendingInviteFriendName`

### New methods (GameLobbyScreen)
- `showInviteConfirmation()`
- `doConfirmInvite()`
- `doCancelInvite()`
- `onConfirmInvite()` / `onCancelInvite()` (static callbacks)

### Behavior changes
- `handleSelect()` (left panel): shows confirm dialog instead of immediately inviting.
- `handleKeyPress()`: routes dialog input while visible.
- `draw()`: draws dialog after lobby UI.
