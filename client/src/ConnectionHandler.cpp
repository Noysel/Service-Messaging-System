#include "../include/ConnectionHandler.h"

using boost::asio::ip::tcp;

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::string;

ConnectionHandler::ConnectionHandler(string host, short port) : host_(host), port_(port), io_service_(),
                                                                socket_(io_service_) {}

ConnectionHandler::~ConnectionHandler() {
    close();
}

bool ConnectionHandler::connect() {
    std::cout << "Starting connect to " << host_ << ":" << port_ << std::endl;
    try {
        tcp::endpoint endpoint(boost::asio::ip::address::from_string(host_), port_); // the server endpoint
        boost::system::error_code error;
        socket_.connect(endpoint, error);
        if (error)
            throw boost::system::system_error(error);
    }
    catch (std::exception &e) {
        std::cerr << "Connection failed (Error: " << e.what() << ')' << std::endl;
        return false;
     }
    return true;
}

bool ConnectionHandler::getBytes(char bytes[], unsigned int bytesToRead) {
    size_t tmp = 0;
    boost::system::error_code error;
    try {
        while (!error && bytesToRead > tmp) {
            tmp += socket_.read_some(boost::asio::buffer(bytes + tmp, bytesToRead - tmp), error);
        }
        if (error)
            throw boost::system::system_error(error);
    } catch (std::exception &e) {
        std::cerr << "recv failed (Error: " << e.what() << ')' << std::endl;
        return false;
    }
    return true;
}

bool ConnectionHandler::sendBytes(const char bytes[], int bytesToWrite) {
    int tmp = 0;
    boost::system::error_code error;
    try {
        while (!error && bytesToWrite > tmp) {
            int sent = socket_.write_some(boost::asio::buffer(bytes + tmp, bytesToWrite - tmp), error);
            tmp += sent;
            std::cout << "[DEBUG] Sent " << sent << " bytes, total sent: " << tmp << "/" << bytesToWrite << std::endl;
        }
        if (error)
            throw boost::system::system_error(error);
    } catch (std::exception &e) {
        std::cerr << "[ERROR] Send failed (Error: " << e.what() << ")" << std::endl;
        return false;
    }
    return true;
}

bool ConnectionHandler::getLine(std::string &line) {
    return getFrameAscii(line, '\0'); // The server's delimiter is '\0'
}

bool ConnectionHandler::sendLine(std::string &line) {
    return sendFrameAscii(line, '\u0000'); // The client's delimiter is '\u0000'
}

bool ConnectionHandler::getFrameAscii(std::string &frame, char delimiter) {
    char ch;
    // Stop when we encounter the null character.
    // Notice that the null character is not appended to the frame string.
    try {
        do {
            if (!getBytes(&ch, 1)) {
                return false;
            }
            if (ch != delimiter) { // Only append if it's not the delimiter
                frame.append(1, ch);
            }
        } while (delimiter != ch);
    } catch (std::exception &e) {
        std::cerr << "recv failed (Error: " << e.what() << ')' << std::endl;
        return false;
    }
    std::cout << "[DEBUG] Received frame: " << frame << " with delimiter: " << delimiter << std::endl;
    return true;
}

bool ConnectionHandler::sendFrameAscii(const std::string &frame, char delimiter) {
    // Sending the frame with the specified delimiter
    //std::cout << "[DEBUG] Preparing to send frame: " << frame << " with delimiter: " << delimiter << std::endl;
    bool result = sendBytes(frame.c_str(), frame.length());
    if (!result) return false;

    char delim[2];
    sprintf(delim, "%c", delimiter);
    std::cout << "[DEBUG] Sending frame: " << frame << " with delimiter: " << delimiter << std::endl;
    std::cout.flush();
    return sendBytes(delim, 1);
}

// Close down the connection properly.
void ConnectionHandler::close() {
    try {
        socket_.close();
    } catch (...) {
        std::cout << "closing failed: connection already closed" << std::endl;
    }
}
