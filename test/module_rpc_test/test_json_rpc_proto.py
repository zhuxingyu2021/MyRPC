import unittest
import random
import socket
import json

port = 8000

class MyTestCase(unittest.TestCase):
    def test_json_rpc_client(self):
        id = 0
        for i in range(3000):
            params = [random.randint(0, 1000) for x in range(100)]
            host = "localhost"

            client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client.connect((host, port))

            # Example echo method
            payload = {
                "jsonrpc": "2.0",
                "id": id,
                "method": "sum",
                "params": params
            }
            client.send(json.dumps(payload).encode('utf-8'))
            response = client.recv(4096).decode('utf-8')
            response = json.loads(response)

            self.assertEqual(response["result"], sum(params))
            self.assertEqual(response["jsonrpc"], "2.0")
            self.assertEqual(response["id"], id)
            id += 1

            client.close()


if __name__ == '__main__':
    unittest.main()
