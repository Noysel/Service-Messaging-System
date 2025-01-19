package bgu.spl.net.srv;

public class User {
    private int connectionId;
    private String loginUser;
    private String passcode;
    private boolean isConnected;
    private final double acceptVersion = 1.2;
    private final String host = "stomp.cs.bgu.ac.il";

    public User(int connectionId, String loginUser, String passcode) {
        this.connectionId = connectionId;
        this.loginUser = loginUser;
        this.passcode = passcode;
        this.isConnected = false;
    }

    public int getConeectionId() {
        return connectionId;
    }

    public String getLoginUser() {
        return loginUser;
    }

    public String getPasscode() {
        return passcode;
    }

    public boolean isConnected() {
        return isConnected;
    }

    public void connect() {
        isConnected = true;
    }

    public void disconnect() {
        isConnected = false;
    }
}
