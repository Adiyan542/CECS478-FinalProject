# Runbook — How to Build and Run the Secure Chat System
CECS 478 – Alpha/Beta Integrated Release

## 1. Before You Start
To run this project, you need:

- macOS, Windows or Linux
- Docker Desktop installed and running
- Python 3 (for the demo scripts)

Check if Docker is running: **docker info**

If it prints version information and does not show errors, Docker is ready.

---

## 2. How To Rebuild the Entire Project (Fresh Clone)

1. Download the repository from GitHub as a ZIP file.
2. Unzip the folder.
3. Open a terminal and go into the project folder: **cd ~/Downloads/CECS478-FinalProject-tofu**
4. Run the full rebuild command: **make up**

This command will:

- Build all Docker containers  
- Recreate the isolated Docker network  
- Compile the server and client programs inside the containers  
- Start all containers in the background  

This process takes less than one minute.

---

## 3. Run the Automatic Demo (Required Vertical Slice)

To show a complete working version of the system: **make demo**

This will:

- Start the server  
- Start all three clients  
- Perform key exchanges  
- Simulate a full encrypted chat session  
- Show a negative test (empty + oversized message)  
- Shut everything down safely  

## 4. Running the Chat Manually (Optional)

### Start the server:
docker exec -it chat_server server 51262

### Start each client:
docker exec -it client1 client 172.16.238.10 51262
docker exec -it client2 client 172.16.238.10 51262
docker exec -it client3 client 172.16.238.10 51262

Once connected, type your name and start chatting.

---

## 5. Capturing Encrypted Traffic (For Evaluation)

Open a new terminal window and run: **docker exec -it chat_server tcpdump -i eth0 port 51262 -A**

This shows encrypted packets.  
You should not see any plaintext messages.

Stop tcpdump: **Ctrl + C**

## 6. Cleaning Up

To stop and remove all containers: **make clean**


## 7. Troubleshooting
### Problem: Docker not running
Open Docker Desktop and wait until it says “Docker Desktop is running.”

### Problem: Clients cannot connect
Make sure the server is actually running: **docker exec -it chat_server server 51262**

This runbook explains everything needed to rebuild and run the system within 5 minutes, as required by the assignment.



