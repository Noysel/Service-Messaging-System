package bgu.spl.net.impl.echo;

import bgu.spl.net.impl.echo.LineMessageEncoderDecoder;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.util.Scanner;

public class EchoClient {

    public static void main(String[] args) throws IOException {

        if (args.length == 0) {
            args = new String[]{"localhost"}; // Default host
        }

        if (args.length < 1) {
            System.out.println("You must supply at least one argument: host");
            System.exit(1);
        }

        try (Socket sock = new Socket(args[0], 7777); // Connect to the server on port 7777
             InputStream in = sock.getInputStream();
             OutputStream out = sock.getOutputStream();
             Scanner sc = new Scanner(System.in)) {

            LineMessageEncoderDecoder encdec = new LineMessageEncoderDecoder(); // Encoder-decoder instance

            String nextMessage;

            do {
                if (sock.isClosed()) {
                    System.out.println("Connection closed by the server.");
                    break;
                }

                System.out.println("Enter a message to send to the server (use \\n for newlines, type 'bye' to quit):");
                nextMessage = sc.nextLine(); // Read user input

                // Replace literal '\n' with actual newlines
                nextMessage = nextMessage.replace("\\n", "\n");

                // Encode and send the message
                out.write(encdec.encode(nextMessage));
                out.flush();

                System.out.println("Awaiting response...");

                // Decode the server's response
                StringBuilder serverMessage = new StringBuilder();
                int read;
                while ((read = in.read()) != -1) {
                    String decoded = encdec.decodeNextByte((byte) read);
                    if (decoded != null) {
                        serverMessage.append(decoded);
                        break; // Stop when a full message is received
                    }
                }

                if (serverMessage.length() == 0) {
                    System.out.println("Connection closed by the server.");
                    break;
                }

                System.out.println("Message from server: ");
                System.out.println(serverMessage.toString());
            } while (!nextMessage.equals("bye") && !sock.isClosed()); // Exit loop when "bye" is entered
        }
    }
}
