package bgu.spl.net.srv;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.List;
import java.util.Set;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.concurrent.CopyOnWriteArraySet;

public class ConnectionsImpl<T> implements Connections<T> {
    private ConcurrentHashMap<Integer, ConnectionHandler<T>> connectionsMap;
    // connects between connectionId and connectionHandler
    private ConcurrentHashMap<String, User> userMap;
    // connects between username and it's data 
    private ConcurrentHashMap<String, Set<Integer>> channelSubscribersMap;
    // connects between topic and connectionIds that subsribed to it
 
    public ConnectionsImpl() {
        connectionsMap = new ConcurrentHashMap<>();
        channelSubscribersMap = new ConcurrentHashMap<>();
    }

    @Override
    public boolean send(int connectionId, T msg) {
        ConnectionHandler<T> handler = connectionsMap.get(connectionId);
        if (handler != null) {
            handler.send(msg);
            return true;
        }
        return false;
    }

    @Override
    public void send(String channel, T msg) {
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
        for (Set<Integer> subscribers : channelSubscribersMap.values()) {
            subscribers.remove(connectionId);
        }
    }

    @Override    
    public void subscribe(String channel, int connectionId) {
        channelSubscribersMap
                .computeIfAbsent(channel, key -> ConcurrentHashMap.newKeySet())
                .add(connectionId);
    }

    
    public void unsubscribe(String channel, int connectionId) {
        Set<Integer> subscribers = channelSubscribersMap.get(channel);
        if (subscribers != null) {
            subscribers.remove(connectionId);
            if (subscribers.isEmpty()) {
                channelSubscribersMap.remove(channel);
            }
        }
    }

    public User getUser(String username) {
        return userMap.get(username);

    }

    public void addUser(User user){
        userMap.put(user.getLoginUser(), user);
        
    }

}
