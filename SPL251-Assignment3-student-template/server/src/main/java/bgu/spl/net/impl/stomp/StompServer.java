package bgu.spl.net.impl.stomp;

import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.impl.echo.EchoProtocol;
import bgu.spl.net.srv.Server;

public class StompServer {

    public static void main(String[] args) {
        if (args.length < 2) {
            System.out.println("Usage: StompServer <port> <tpc/reactor>");
            return;
        }
        int port = Integer.parseInt(args[0]);
        String mode = args[1];

        if (mode.equals("tpc")) {
            Server.threadPerClient(
                    port,
                    () -> new StompMessagingProtocolImpl(), // protocol factory
                    StompEncoderDecoderImpl::new // Your encoder-decoder implementation
            ).serve();
        } else if (mode.equals("reactor")) {
            int numThreads = 3; //check 
            Server.reactor(
                    numThreads,
                    port,
                    StompMessagingProtocolImpl::new, // Your protocol implementation
                    StompEncoderDecoderImpl::new // Your encoder-decoder implementation
            ).serve();
        } else {
            System.out.println("Invalid mode. Use 'tpc' or 'reactor'.");
        }
    }
}
