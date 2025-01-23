package bgu.spl.net.srv;


public interface Connections<T> {

    boolean send(int connectionId, T msg);

    void send(String channel, String subscriptionId, T msg);

    //send(destination, subscriptionId, body)

    void disconnect(int connectionId);

    void subscribe(String channel, int connectionId);

    void unsubscribe(String channel, int connectionId);

    User getUser(String username);

    void addUser(User user);

    boolean isSubscribedToChannel(int connectionId, String channel);

    void addConnection(int connectionId, ConnectionHandler<T> handler);

    public int getAndIncrementMessageId();
}
