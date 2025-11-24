# 🚨 Emergency Service Messaging System

A cross-platform emergency-event messaging platform implemented as part of the SPL_251 Systems Programming course at Ben-Gurion University. The system enables users to connect, authenticate, subscribe to emergency channels, report events, and receive real-time updates using the STOMP 1.2 protocol.

## 🔧 Technologies Used

* **Java** (Server logic, Reactor & TPC networking)
* **C++** (Client application, socket communication)
* **STOMP 1.2 Protocol** over TCP
* **Multithreading** (mutex synchronization for client I/O)
* **Reactor Pattern & Thread-Per-Client architectures**
* **JSON Parsing** for event files
* **Linux Socket Programming**
* **Docker compatible environment**

## 💡 Project Structure

### **Client (C++)**

Handles:

* User command interpretation
* STOMP frame construction & parsing
* Socket communication lifecycle
* Multithreaded keyboard/socket input
* Local emergency event storage and summary generation

Supported Commands:
`login`, `join`, `exit`, `report`, `summary`, `logout`

### **Server (Java)**

* Based on `bgu.spl.net.Server` template provided in course
* Supports: **Thread Per Client (TPC)** & **Reactor (NIO)** modes
* Validates clients, manages users, tracks subscriptions, routes STOMP frames
* Persists user credentials (keeps passwords after logout)

### **Root Directory**

```
.
├── client/
│   ├── bin/
│   ├── data/
│   ├── include/
│   ├── src/
│   └── makefile
├── server/
│   └── src/main/java/bgu/spl/net/
│   └── pom.xml
├── events1.json
├── events1_out.txt
├── README.md
```

## ✨ Features

* Full **STOMP 1.2** compliance
* Real-time event broadcasting to all subscribers
* Username authentication + password persistence
* Multithreaded client I/O (keyboard + socket listener)
* Topic-based publish-subscribe emergency channels
* Summary generation based on user events
* Graceful disconnect with STOMP `RECEIPT`

## 🚀 How to Run

### **Run Server**

```
cd server/
mvn clean compile
mvn exec:java -Dexec.mainClass="bgu.spl.net.impl.stomp.StompServer" -Dexec.args="7777 tpc"
# OR Reactor mode
mvn exec:java -Dexec.mainClass="bgu.spl.net.impl.stomp.StompServer" -Dexec.args="7777 reactor"
```

### **Run Client**

```
cd client/
make
cd bin
./StompClient
```

### **Example Run**

**Terminal 1 (Alice)**

```
login 127.0.0.1:7777 Alice 123
join police
```

**Terminal 2 (Bob)**

```
login 127.0.0.1:7777 Bob abc
join police
report /path/events1.json
```

**Back to Terminal 1**

```
summary police Bob /path/events1_out.txt
logout
```

**Terminal 2**

```
logout
```

## 🖥️ Client Command Reference

| Command                             | Description                              |
| ----------------------------------- | ---------------------------------------- |
| `login <host>:<port> <user> <pass>` | Connect & authenticate to the server     |
| `join <topic>`                      | Subscribe to a channel                   |
| `exit <topic>`                      | Unsubscribe from a channel               |
| `report <file>`                     | Send all events from JSON file to server |
| `summary <topic> <user> <file>`     | Export collected events of a user        |
| `logout`                            | Gracefully disconnect from the server    |

## 📊 Event File Format

```json
{
  "channel_name": "<topic>",
  "events": [
    {
      "event_name": "<string>",
      "city": "<string>",
      "date_time": <unix_timestamp>,
      "description": "<string>",
      "general_information": {
        "active": <boolean>,
        "forces_arrival_at_scene": <boolean>
      }
    }
  ]
}
```

* `channel_name` defines the topic the events belong to
* `date_time` handled as UNIX timestamp
* `active` and `forces_arrival_at_scene` stored and summarized

## 🧪 Tests & Debugging

* Verified using multiple simulated users & channels
* Client memory safety validated (Linux + Valgrind)
* Optional debug prints in server for message flow

## 📚 Course Information

* **Course:** SPL 251 – Systems Programming
* **Institution:** Ben-Gurion University of the Negev (BGU)
* **Year:** 2025
* **Environment:** Linux (Lab Machines / Docker compatible)

## 🧑‍💻 Authors

* **Lior Lotan** – [LinkedIn](https://www.linkedin.com/in/lior-lotan/)
* **Noy Sela** – [LinkedIn](https://www.linkedin.com/in/noy-sela-659a32366/)

---

### 📝 Important Note

Ensure JSON formatting is valid and file paths are correct when running outside the provided Docker environment. The project demonstrates real-time publish-subscribe messaging using socket programming (C++) and concurrent server design (Java).
