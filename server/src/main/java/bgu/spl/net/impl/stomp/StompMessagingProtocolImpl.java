package bgu.spl.net.impl.stomp;

import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.Connections;
import bgu.spl.net.srv.User;

import java.util.concurrent.ConcurrentHashMap;

public class StompMessagingProtocolImpl implements StompMessagingProtocol<String> {
    private int connectionId;
    private Connections<String> connections;
    private boolean shouldTerminate = false;
    private ConcurrentHashMap<String, String> subscribersChannelsMap = new ConcurrentHashMap<>();
    // key is subscriptionID, value is channel
    private User currentUser = null;

    @Override
    public void start(int connectionId, Connections<String> connections) {
        this.connectionId = connectionId;
        this.connections = connections;
        System.out.println("protocol started!");
    }

    @Override
    public void process(String message) {
        String[] lines = message.split("\n");
        String msgType = lines[0];

        if (msgType.equals("CONNECT")) {
            handleConnect(lines);
                return;
        }
        else if (currentUser == null) {
                handleError("Cannot preform the action because user is not connected");
                return;
            }

        switch (msgType) {
            case "SEND":
                handleSend(lines);
                break;

            case "SUBSCRIBE":
                handleSubscribe(lines);
                break;

            case "UNSUBSCRIBE":
                handleUnsubscribe(lines);
                break;

            case "DISCONNECT":
                handleDisconnect(lines);
                break;

            default:
                handleError("Unknown command: " + msgType);
        }
    }

    @Override
    public boolean shouldTerminate() {
        return shouldTerminate;
    }

    private void handleConnect(String[] lines) {
        System.out.println("Handle Connect");
        String username = getHeaderVal(lines, "login");
        String passcode = getHeaderVal(lines, "passcode");
        User user = connections.getUser(username);
        if (user == null) {
            User newUser = new User(connectionId, username, passcode);
            connections.addUser(newUser);
            newUser.connect();
            currentUser = newUser;
            connections.send(connectionId, "CONNECTED\nversion:1.2\n\n\0");
            return;
        }
        if (!user.getPasscode().equals(passcode)) {
            handleError("Wrong password");
            return;
        }
        if (user.isConnected()) {
            handleError("Already logged in");
            return;
        }
        user.connect();
        currentUser = user;
        connections.send(connectionId, "CONNECTED\nversion:1.2\n\n\0");
    }

    private void handleSubscribe(String[] lines) {
        String destination = getHeaderVal(lines, "destination");
        String subscriptionId = getHeaderVal(lines, "id");
        String receiptId = getHeaderVal(lines, "receipt");
        if (destination == null || subscriptionId == null) {
            handleError("Missing 'destination' or 'id' header in SUBSCRIBE");
            return;
        }
        if (subscribersChannelsMap.containsKey(subscriptionId)) {

            handleError("Subscription ID '" + subscriptionId + "' is already subscribed to '" + destination);
            return;
        }
        subscribeToChannel(destination, subscriptionId);
        System.out.println("subChannelMap in protocol" + subscribersChannelsMap.toString());
        if (receiptId != null) {
            connections.send(connectionId, "RECEIPT\nreceipt-id:" + receiptId + "\n\n\0");
        }
    }

    private void handleSend(String[] lines) {
        String destination = getHeaderVal(lines, "destination");
        System.out.println("destination in send: " + destination);
        String body = getMessageBody(lines);
        if (destination == null || body == null) {
            handleError("Missing 'destination' or body in SEND");
            return;
        }
        if (!connections.isSubscribedToChannel(connectionId, destination)) {
            handleError("User is not subscribed to channel");
            return;
        }
        String response = "MESSAGE\ndestination:" + destination + "\n\n" + body + "\0";
        connections.send(destination, response);
    }

    private void handleUnsubscribe(String[] lines) {
        String subscriptionId = getHeaderVal(lines, "id");
        String receiptId = getHeaderVal(lines, "receipt");

        if (subscriptionId == null) {
            connections.send(connectionId, "ERROR\nmessage:Missing 'id' header in UNSUBSCRIBE\n\n\0");
            return;
        }
        String destination = subscribersChannelsMap.get(subscriptionId);
        if (destination == null) {
            handleError("User is not subscribed to channel");
            return;
        }
        unsubscribeToChannel(destination, subscriptionId);
        if (receiptId != null) {
            String receiptFrame = "RECEIPT\nreceipt-id:" + receiptId + "\n\n\0";
            connections.send(connectionId, receiptFrame);
        }
    }

    private void handleDisconnect(String[] lines) {
        String receiptId = getHeaderVal(lines, "receipt");
        disconnect();
        connections.send(connectionId, "RECEIPT\nreceipt-id:" + receiptId + "\n\n\0");
    }

    private void handleError(String errorMessage) {

        System.out.println("Handle Error");
        String response = "ERROR\nmessage: " + errorMessage + "\n\n\0";
        System.out.println("From handleError: " + response);
        connections.send(connectionId, response);
        try {
            Thread.sleep(300); // 100 ms delay
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
        disconnect();
    }

    // ***methods for handeling messages properly***

    private String getHeaderVal(String[] lines, String header) {
        for (String line : lines) {
            if (line.startsWith(header + ":")) {
                return line.substring(header.length() + 1).trim();
            }
        }
        return null;
    }

    public void subscribeToChannel(String channel, String subscriptionId) {
        subscribersChannelsMap.put(subscriptionId, channel);
        connections.subscribe(channel, connectionId);
    }

    public void unsubscribeToChannel(String channel, String subscriptionId) {
        subscribersChannelsMap.remove(subscriptionId);
        connections.unsubscribe(channel, connectionId);
    }

    private String getMessageBody(String[] lines) {
        int emptyLineIndex = -1;
        for (int i = 0; i < lines.length; i++) {
            if (lines[i].isEmpty()) {
                emptyLineIndex = i;
                break;
            }
        }
        if (emptyLineIndex != -1 && emptyLineIndex < lines.length - 1) {
            return lines[emptyLineIndex + 1].trim();
        }
        return null;
    }

    public void disconnect() {
        connections.disconnect(connectionId);
        if (currentUser != null) {
            currentUser.disconnect();
        }
        shouldTerminate = true;
    }
}
