package bgu.spl.net.srv;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.Set;

public class ConnectionsImpl<T> implements Connections<T> {
    private ConcurrentHashMap<Integer, ConnectionHandler<T>> connectionsMap;
    // connects between connectionId and connectionHandler
    private ConcurrentHashMap<String, User> userMap;
    // connects between username and it's data 
    private ConcurrentHashMap<String, Set<Integer>> channelSubscribersMap;
    // connects between topic and connectionIds that subsribed to it
    private AtomicInteger messageIDgenerator = new AtomicInteger(0);
 
    public ConnectionsImpl() {
        connectionsMap = new ConcurrentHashMap<>();
        channelSubscribersMap = new ConcurrentHashMap<>();
        userMap = new ConcurrentHashMap<>();
    }

    @Override
    public boolean send(int connectionId, T msg) {
        System.out.println("Connection Map status: " + connectionsMap.toString());
        ConnectionHandler<T> handler = connectionsMap.get(connectionId);
        if (handler != null) {
            System.out.println("Connections send: " + msg);
            handler.send(msg);
            return true;
        }
        return false;
    }

    @Override
    public void send(String channel, T msg) {
        System.out.println("Connection Map status: " + connectionsMap.toString());
        Set<Integer> subscribers = channelSubscribersMap.get(channel);
        if (subscribers != null) {
            for (Integer connectionId : subscribers) {
                send(connectionId, msg);
            }
        }
    }

    @Override
    public void disconnect(int connectionId) {
        connectionsMap.remove(connectionId);
        for (String channel : channelSubscribersMap.keySet()) {
            unsubscribe(channel, connectionId);
        }
    }

    @Override    
    public void subscribe(String channel, int connectionId) {
        channelSubscribersMap
                .computeIfAbsent(channel, key -> ConcurrentHashMap.newKeySet())
                .add(connectionId);
        
        System.out.println("ChannelSubscribeMap in Connections" + channelSubscribersMap.toString());
    }

    @Override
    public void unsubscribe(String channel, int connectionId) {
        synchronized (channelSubscribersMap) {
            Set<Integer> subscribers = channelSubscribersMap.get(channel);
            if (subscribers != null) {
                subscribers.remove(connectionId);
                if (subscribers.isEmpty()) {
                    channelSubscribersMap.remove(channel);
                }
            }
        }
    }

    @Override    
    public User getUser(String username) {
        return userMap.get(username);

    }

    @Override
    public void addUser(User user){
        userMap.put(user.getLoginUser(), user);
        
    }

    @Override
    public boolean isSubscribedToChannel (int connectionId, String channel) {
        Set<Integer> subscribers = channelSubscribersMap.get(channel);
       //System.out.println("subscribers: " + subscribers.toString());
        if (subscribers == null || !subscribers.contains(connectionId)) {
            //System.out.println("FALSEEEEEEEEEEEEEEe");
            return false;
        }
        return true;
    }

    public void addConnection(int connectionId, ConnectionHandler<T> handler) {
        connectionsMap.put(connectionId, handler);
        System.out.println("Connections added a new connection!");
    }

    public int getAndIncrementMessageId() {
        return messageIDgenerator.getAndIncrement();
    }

}
