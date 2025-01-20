package bgu.spl.net.srv;

import bgu.spl.net.api.MessageEncoderDecoder;
import bgu.spl.net.api.MessagingProtocol;
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.IOException;
import java.net.Socket;

public class BlockingConnectionHandler<T> implements Runnable, ConnectionHandler<T> {

    private final MessagingProtocol<T> protocol;
    private final MessageEncoderDecoder<T> encdec;
    private final Socket sock;
    private BufferedInputStream in;
    private BufferedOutputStream out;
    private volatile boolean connected = true;

    public BlockingConnectionHandler(Socket sock, MessageEncoderDecoder<T> reader, MessagingProtocol<T> protocol) {
        this.sock = sock;
        this.encdec = reader;
        this.protocol = protocol;
    }

    @Override
    public void run() {
        try (Socket sock = this.sock) { // just for automatic closing
            int read;

            in = new BufferedInputStream(sock.getInputStream());
            out = new BufferedOutputStream(sock.getOutputStream());

            System.out.println("Connection Handler - before while");
            while (!protocol.shouldTerminate() && connected && (read = in.read()) >= 0) {
                System.out.println("Connection Handler - inside while");
                T nextMessage = encdec.decodeNextByte((byte) read);
                System.out.println("NextMessage: " + nextMessage);
                if (nextMessage != null) {
                    protocol.process(nextMessage); // T response =
                    /*  
                    System.out.println("Response: " + response);
                    if (response != null) {
                        System.out.println("Sent by process: " + response);
                        out.write(encdec.encode(response));
                        out.flush();
                    }
                        */
                }
            }

        } catch (IOException ex) {
            ex.printStackTrace();
        } finally {
            try {
                close(); // Explicitly close resources
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

    }

    @Override
    public void close() throws IOException {
        connected = false;
        sock.close();
    }

    @Override
    public void send(T msg) {
        if (!connected || sock.isClosed()) {
            System.out.println("Attempted to send a message on a closed connection. Ignoring.");
            return; // Avoid sending if the connection is already closed
        }
    
        try {
            synchronized (out) { // Ensure thread-safety on the output stream
                out.write(encdec.encode(msg));
                out.flush(); // Flush to guarantee the message is sent
            }
        } catch (IOException e) {
            System.out.println("Error during send: " + e.getMessage());
            try {
                close(); // Ensure the connection is properly closed
            } catch (IOException closeException) {
                closeException.printStackTrace();
            }
        }
    }
}
