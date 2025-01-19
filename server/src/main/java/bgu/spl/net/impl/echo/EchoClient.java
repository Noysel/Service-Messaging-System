package bgu.spl.net.impl.echo;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.Socket;
import java.util.Scanner;

public class EchoClient {

    public static void main(String[] args) throws IOException {

        if (args.length == 0) {
            args = new String[]{"localhost"}; // Default host
        }

        if (args.length < 1) {
            System.out.println("you must supply at least one argument: host");
            System.exit(1);
        }
        try (Socket sock = new Socket("localhost", 7777);
        BufferedReader in = new BufferedReader(new InputStreamReader(sock.getInputStream()));
        BufferedWriter out = new BufferedWriter(new OutputStreamWriter(sock.getOutputStream()));
        Scanner sc = new Scanner(System.in)) { // Use a single Scanner instance

       String nextMessage;

       do {
           System.out.println("Enter a message to send to the server (type 'bye' to quit):");
           nextMessage = sc.nextLine(); // Read user input
           out.write(nextMessage + "\u0000"); // Append the delimiter
           out.flush(); // Send the message

           System.out.println("awaiting response");
           String line = in.readLine(); // Read server response
           System.out.println("message from server: " + line);
       } while (!nextMessage.equals("bye") && !sock.isClosed()); // Continue until "bye" is entered
   }
    }
}
