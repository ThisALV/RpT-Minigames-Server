import asyncio
import signal
import ssl
import websockets
from sys import argv


url = argv[1]  # 1st argument is wss:// URL for game server
certificate = argv[2]  # 2nd argument is path to custom certificate required for localhost self-signed PEM

# Enables TLS features
tls_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
tls_context.keylog_filename = "keylog.pem"  # Wireshark will need that for debugging
tls_context.load_verify_locations(certificate)  # localhost requires self-signed PEM cert


def require_input(input_required: asyncio.Event):
    print("\b\b", end="")
    input_required.set()


async def send_messages(connection: websockets.WebSocketClientProtocol, console_lock: asyncio.Lock, input_required: asyncio.Event):
    """Wait for the console to be available then read input message from stdin and send it to server."""

    while connection.open:  # Waits for next Ctrl+C until connection has been closed
        try:
            await input_required.wait()  # Waits for Ctrl+C

            # input_required might have been triggered by connection closure inside receiving coroutine
            if connection.open:
                async with console_lock:  # Waits for the console to be available before reading stdin
                    rptl_message = input("Send: ")

                await connection.send(rptl_message)

            input_required.clear()
        except websockets.ConnectionClosed as closed:  # Connection will end up closed
            print(f"Connection was closed: {closed.reason}")


async def receive_messages(connection: websockets.WebSocketClientProtocol, console_lock: asyncio.Lock, input_required: asyncio.Event):
    """Listen for next RPTL message and wait for the console to be available then displays it to stdout."""

    while connection.open:  # Waits for next RPTL message until connection has been closed
        try:
            rptl_message = await connection.recv()

            async with console_lock:  # Waits for the console to be available before writing to stdout
                print(f"Recv: {rptl_message}")
        except websockets.ConnectionClosed as closed:  # Connection will end up closed
            print(f"Connection was closed: {closed.reason}")

    require_input(input_required)  # Stops waiting for Ctrl+C to send message as connection has been closed


async def rptl_connection(connection: websockets.WebSocketClientProtocol):
    """Listen for RPTL message on given connection and displays them to stdout.
    At SIGINT, messages no longer displayed until RPTL message has been written to stdin and sent to server."""

    console_lock = asyncio.Lock()  # To not write a received message and read a sending message simultaneously
    input_required = asyncio.Event()  # To notify the task sending message that it should prompt for message into stdin

    asyncio.get_running_loop().add_signal_handler(signal.SIGINT, require_input, input_required)  # When Ctrl+C is triggered, prompt for RPTL message
    receiving = asyncio.create_task(receive_messages(connection, console_lock, input_required))
    sending = asyncio.create_task(send_messages(connection, console_lock, input_required))

    await asyncio.gather(receiving, sending)


async def websocket_connection(host_url: str, security_context: ssl.SSLContext):
    # Wait for WSS client connection to be made
    connection = await websockets.connect(uri=host_url, ssl=security_context)

    print(f"Connected to {host_url}.")

    # Once connection is made, starts RPTL protocol until disconnection
    await rptl_connection(connection)


asyncio.run(websocket_connection(url, tls_context))
