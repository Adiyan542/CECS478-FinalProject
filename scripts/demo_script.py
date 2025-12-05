import subprocess
import time
import sys
import signal


def run_simulation():
    print("Starting Simulation")
    
    # List to keep track of processes to clean up later
    procs = []

    try:
        # Start the Server (Background)
        print("Starting Chat Server")
        server_cmd = ["docker", "exec", "-i", "chat_server", "server", "51262"]
        server_proc = subprocess.Popen(server_cmd, stdin=subprocess.PIPE, stdout=None, stderr=None)
        procs.append(server_proc)
        time.sleep(1)

        # 2. Client 1 Joins
        print("Client 1 connecting...")
        c1_cmd = ["docker", "exec", "-i", "client1", "client", "172.16.238.10", "51262"]
        # IMPORTANT: Redirect client output to /dev/null to hide it
        c1_proc = subprocess.Popen(c1_cmd, stdin=subprocess.PIPE, 
                                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        procs.append(c1_proc)
        time.sleep(1)

        # Client 1 Enters Name (needs newline!)
        c1_proc.stdin.write(b"Bob\n")
        c1_proc.stdin.flush()
        time.sleep(1)

        # Client 1 Chats
        c1_proc.stdin.write(b"Hello everyone! I am Bob.\n")
        c1_proc.stdin.flush()
        time.sleep(1)

        # Client 2 Joins
        print("Client 2 connecting...")
        c2_cmd = ["docker", "exec", "-i", "client2", "client", "172.16.238.10", "51262"]
        c2_proc = subprocess.Popen(c2_cmd, stdin=subprocess.PIPE,
                                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        procs.append(c2_proc)
        time.sleep(1)

        # Client 2 Enters Name
        c2_proc.stdin.write(b"Alice\n")
        c2_proc.stdin.flush()
        time.sleep(1)

        # Interleaved Chatting between Client 1 and 2
        print("--- Chatting ensues ---")
        c2_proc.stdin.write(b"Hi Bob! Alice here.\n")
        c2_proc.stdin.flush()
        time.sleep(0.5)
        
        c1_proc.stdin.write(b"Nice to meet you, Alice.\n")
        c1_proc.stdin.flush()
        time.sleep(1)

        # Client 3 Joins
        print("Client 3 connecting")
        c3_cmd = ["docker", "exec", "-i", "client3", "client", "172.16.238.10", "51262"]
        c3_proc = subprocess.Popen(c3_cmd, stdin=subprocess.PIPE,
                                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        procs.append(c3_proc)
        time.sleep(0.5)

        # Client 3 Enters Name
        c3_proc.stdin.write(b"David\n")
        c3_proc.stdin.flush()
        time.sleep(1)

        c3_proc.stdin.write(b"Hello, am I late?\n")
        c3_proc.stdin.flush()
        time.sleep(1)

        # Chatting between all clients
        c1_proc.stdin.write(b"No, you're fine. We just got here.\n")
        c1_proc.stdin.flush()
        time.sleep(1)

        c2_proc.stdin.write(b"Yeah.\n")
        c2_proc.stdin.flush()
        time.sleep(1)

        c3_proc.stdin.write(b"Okay, nice!\n")
        c3_proc.stdin.flush()
        time.sleep(2)

        # Everyone starts to exit
        print("Client 1 begins to exit...")
        c1_proc.stdin.write(b"/exit\n")
        c1_proc.stdin.flush()
        time.sleep(1)

        print("Client 2 begins to exit...")
        c2_proc.stdin.write(b"/exit\n")
        c2_proc.stdin.flush()
        time.sleep(1)

        print("Client 3 begins to exit...")
        c3_proc.stdin.write(b"/exit\n")
        c3_proc.stdin.flush()
        time.sleep(1)

        print("Server begins to exit...")
        print("\n --- Server Logs --- ")
        # Send SIGINT directly to the server process inside the container
        subprocess.run(["docker", "exec", "chat_server", "pkill", "-SIGINT", "server"])
        time.sleep(2)

        try:
            server_proc.wait(timeout=2.5)
            print("Server terminated gracefully.")
        except subprocess.TimeoutExpired:
            print("Server did not exit; forcing kill.")
            server_proc.kill()

    except Exception as e:
        print(f"An error occurred: {e}")

    finally:
        print("\nTerminating all clients and server")
        # Close standard inputs (simulates Ctrl+D or EOF)
        for p in procs:
            if p.stdin:
                p.stdin.close()
            p.terminate()
            try:
                p.wait(timeout=2)
            except subprocess.TimeoutExpired:
                p.kill()
        print("--- Simulation Complete ---")

if __name__ == "__main__":
    run_simulation()
