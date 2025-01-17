// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#pragma once

#include "../../sample.h"

namespace dviglo
{

class Button;
class LineEdit;
class Text;
class UiElement;

}

/// Chat example
/// This sample demonstrates:
///     - Starting up a network server or connecting to it
///     - Implementing simple chat functionality with network messages
class Chat : public Sample
{
    DV_OBJECT(Chat);

public:
    SlotLogMessage log_message;

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Create the UI.
    void create_ui();
    /// Subscribe to log message, UI and network events.
    void subscribe_to_events();
    /// Create a button to the button container.
    Button* CreateButton(const String& text, int width);
    /// Print chat text.
    void ShowChatText(const String& row);
    /// Update visibility of buttons according to connection and server status.
    void UpdateButtons();
    /// Handle log message event; pipe it also to the chat display.
    void handle_log_message(const String& message, i32 level);
    /// Handle pressing the send button.
    void HandleSend(StringHash eventType, VariantMap& eventData);
    /// Handle pressing the connect button.
    void HandleConnect(StringHash eventType, VariantMap& eventData);
    /// Handle pressing the disconnect button.
    void HandleDisconnect(StringHash eventType, VariantMap& eventData);
    /// Handle pressing the start server button.
    void HandleStartServer(StringHash eventType, VariantMap& eventData);
    /// Handle an incoming network message.
    void HandleNetworkMessage(StringHash eventType, VariantMap& eventData);
    /// Handle connection status change (just update the buttons that should be shown.)
    void HandleConnectionStatus(StringHash eventType, VariantMap& eventData);
    /// Strings printed so far.
    Vector<String> chatHistory_;
    /// Chat text element.
    SharedPtr<Text> chatHistoryText_;
    /// Button container element.
    SharedPtr<UiElement> buttonContainer_;
    /// Server address / chat message line editor element.
    SharedPtr<LineEdit> textEdit_;
    /// Send button.
    SharedPtr<Button> sendButton_;
    /// Connect button.
    SharedPtr<Button> connectButton_;
    /// Disconnect button.
    SharedPtr<Button> disconnectButton_;
    /// Start server button.
    SharedPtr<Button> startServerButton_;
};
