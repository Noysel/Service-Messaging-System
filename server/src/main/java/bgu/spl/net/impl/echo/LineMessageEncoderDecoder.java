package bgu.spl.net.impl.echo;

import bgu.spl.net.api.MessageEncoderDecoder;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

public class LineMessageEncoderDecoder implements MessageEncoderDecoder<String> {

    private byte[] bytes = new byte[1 << 10]; // Start with 1k buffer
    private int len = 0;

    @Override
    public String decodeNextByte(byte nextByte) {
        // '\0' marks the end of a message
        if (nextByte == '\0') {
            return popString();
        }

        pushByte(nextByte);
        return null; // Not a complete message yet
    }

    @Override
    public byte[] encode(String message) {
        // Append '\0' to the message
        return (message + "\0").getBytes(StandardCharsets.UTF_8);
    }

    private void pushByte(byte nextByte) {
        if (len >= bytes.length) {
            bytes = Arrays.copyOf(bytes, len * 2); // Expand buffer if needed
        }
        bytes[len++] = nextByte;
    }

    private String popString() {
        String result = new String(bytes, 0, len, StandardCharsets.UTF_8);
        len = 0;
        return result;
    }
}
