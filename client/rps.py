import socket
import sys
from os import system, name 

HOST = "127.0.0.1"
PORT = 49152

def main():
	with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
		# connect_to_server(s)
		s.connect((HOST, PORT))

		# send game id to server
		s.sendall(bytearray([0,0,0,0,1,1,2,1,2]))

		# receive uid or error from server always receive 7
		uid = recv_response(s, 7)[3:]
		print("UID: ", uid)

		print("Waiting for server!")

		# Get start signal
		start_signal = recv_response(s, 3)
		print(start_signal)
		
		# send move
		move = prompt_send_data(s, uid)

		print("waiting for confirmation!")
		response = recv_response(s, 3)

		print("Waiting for result:")
		result = recv_response(s, 5)
		print("You played: ", end="")
		print("Rock" if move == 1 else "Paper" if move == 2 else "Scissors")
		print("Opponent played: ", end="")
		print("Rock" if result[4] == 1 else "Paper" if result[4] == 2 else "Scissors")
		print("======>", end="")
		print("You won!" if result[3] == 1 else "You lost!" if result[3] == 2 else "Tie game!")
		s.close()

# Communication
def recv_response(s, n):
	response = [ord(str(s.recv(1), "utf-8")[0]) for _ in range(n)]
	print("Response:: ", response)
	return response

def prompt_send_data(s, uid):
	while 1:
		toSend = input("\n[Make your move](1-3): ")
		if toSend == "end":
			system(exit)
		try:
			toSend = int(toSend)
			if toSend < 1 or toSend > 3:
				print("That was not a valid move")
				continue
			break
		except:
			print("Not a valid option")
			continue
	s.sendall(bytearray(uid + [4, 1, 1, toSend]))
	return toSend

if __name__ == "__main__":
    main()
