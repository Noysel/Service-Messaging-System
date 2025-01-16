package bgu.spl.net.impl.stomp;

import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.Connections;
import bgu.spl.net.srv.ConnectionsImpl;
import bgu.spl.net.srv.User;

import java.util.concurrent.ConcurrentHashMap;

public class StompMessagingProtocolImpl implements StompMessagingProtocol<String> {
    private int connectionId;
    private Connections<String> connections;
    private boolean shouldTerminate = false;
    private ConcurrentHashMap<String, String> subscribersChannelsMap = new ConcurrentHashMap<>();
    // key is subscriptionID, value is channel

    @Override
    public void start(int connectionId, Connections<String> connections) {
        this.connectionId = connectionId;
        this.connections = connections;
    }

    @Override
    public String process(String message) {
        String[] lines = message.split("\n");
        String msgType = lines[0];

        switch (msgType) {
            case "CONNECT":
                handleConnect(lines);
                break;

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
                handleDisconnect();
                break;
            default:
                handleError("Unknown command: " + msgType);
                break;
        }
        return null; //
    }

    @Override
    public boolean shouldTerminate() {
        return shouldTerminate;
    }

    private String handleConnect(String[] lines) {
        String username = getHeaderVal(lines, "login");
        String passcode = getHeaderVal(lines, "passcode");
        User user = connections.getUser(username);
        if (user == null){
            User newUser = new User(User.getNewConnectionId(), username, passcode);
            connections.addUser(newUser);
            newUser.connect();
            connections.send(connectionId, "CONNECTED\nversion:1.2\n\n\0");
            return "CONNECTED\nversion:1.2\n\n\0";
        }
        if (user.getPasscode() != passcode) {
            return handleError("Wrong password");
        }
        if (user.isConnected()){
            return handleError("Already logged in");
        }
        user.connect();
        connections.send(connectionId, "CONNECTED\nversion:1.2\n\n\0");
        return "CONNECTED\nversion:1.2\n\n\0";
        
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
        if (receiptId != null) {
            connections.send(connectionId, "RECEIPT\nreceipt-id:" + receiptId + "\n\n\0");
        }

    }

    private void handleSend(String[] lines) {
        String destination = getHeaderVal(lines, "destination");
        String body = getMessageBody(lines);
        if (destination == null || body == null) {
            handleError("Missing 'destination' or body in SEND");
            return;
        }
        String response = "MESSAGE\ndestination:" + destination + "\n\n" + body + "\0";
        connections.send(destination, response);

        //add error if client doesn't subscribed to destination
    }

    private void handleUnsubscribe(String[] lines) {
        String subscriptionId = getHeaderVal(lines, "id");
        String receiptId = getHeaderVal(lines, "receipt");

        if (subscriptionId == null) {
            // Send an ERROR frame if the id header is missing
            connections.send(connectionId, "ERROR\nmessage:Missing 'id' header in UNSUBSCRIBE\n\n\0");
            return;
        }
        String destination = subscribersChannelsMap.get(subscriptionId);
        if (destination == null) {
            handleError("Invalid 'id' in UNSUBSCRIBE: Subscription not found");
            return;
        }
        unsubscribeToChannel(destination, subscriptionId);
        if (receiptId != null) {
            String receiptFrame = "RECEIPT\nreceipt-id:" + receiptId + "\n\n\0";
            connections.send(connectionId, receiptFrame);
        }
    }

    private void handleDisconnect() {
        connections.disconnect(connectionId);
        shouldTerminate = true;
    }

    private String handleError(String errorMessage) {
        String response = "ERROR\nmessage: " + errorMessage + "\n\n\0";
        connections.send(connectionId, "ERROR\nmessage:" + errorMessage + "\n\n\0");
        shouldTerminate = true;
        return response;
    }

    // methods for handeling messages properly

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
}
