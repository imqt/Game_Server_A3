import socket
import sys
from os import system, name 
from datetime import datetime

HOST = "127.0.0.1"
PORT = 49152

AUTO_HOST = "";
AUTO_PORT = 0;

def main():
	print("TicTacToe Python client!\n")
	print("Client Start Time =", datetime.now().strftime("%H:%M:%S"))
	
	with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
		connect_to_server(s)
		print("Connected Time =", datetime.now().strftime("%H:%M:%S"))

		# send game id to server 
		s.sendall(bytearray([0,0,0,0,1,1,2,1,1]))

		# receive uid or error from server always receive 7
		uid = recv_response(s, 7)[3:]
		print("UID: ", uid)

		# Initialize board
		board = ["-" for x in range(9)]
		display_game_screen(board)
		print("Waiting for server!")

		# Get start signal
		play_turn = recv_response(s, 4)[3] - 1
		turn_counter = 0;
		if (play_turn == 0):
			print("You are playing X")
			move = prompt_send_data(s, uid, board) # send move
			recv_response(s, 3) # wait for confirmation
			update_board(board, move, turn_counter)
			turn_counter += 1
		else:
			print("You are playing O")

		# Game loop
		game_state = 1
		while game_state != 2:
			print("Waiting for " + ("X" if play_turn else "O") + " player!")
			game_state = recv_response(s, 3)[2] # get response type (1=> opponent's move, 2=> game ended)

			if game_state == 0:
				print("You won, the other client quit!")
			elif game_state == 1:
				update_board(board, recv_response(s, 1)[0], turn_counter)
				turn_counter += 1
				move = prompt_send_data(s, uid, board) # sendmove
				recv_response(s, 3) # wait for confirmation
				update_board(board, move, turn_counter)
				turn_counter += 1
			else:
				turn_counter += 0 if turn_counter%2!=play_turn else 1
				result = recv_response(s, 1)[0]
				move = recv_response(s, 1)[0]
				update_board(board, move, turn_counter)
				print("======>", end="")
				print("You won!" if result == 1 else "You lost!" if result == 2 else "Tie game!")
		s.close()

# UI
def display_game_screen(board):
	print("\nGuide board: ", end="")
	for index in range(len(board)):
		if index%3==0: 
			print()
		print(index, " ", end="")
	print("\nCurrent game: ", end="")
	for index in range(len(board)):
		if index%3==0: 
			print()
		print(board[index], " ", end="")
	print()

def update_board(board, move, turn_counter):
	board[move] = "X" if turn_counter%2==0 else "O"
	display_game_screen(board)

# Communication
def recv_response(s, n):
	response = [ord(str(s.recv(1), "utf-8")[0]) for _ in range(n)]
	print("Response:: ", response)
	return response

def prompt_send_data(s, uid, board):
	while 1:
		toSend = input("\n[Make your move](0-8): ")
		if toSend == "end":
			system(exit)
		try:
			toSend = int(toSend)
			if toSend < 0 or toSend > 8 or board[toSend] != '-':
				print("That was not a valid move")
				continue 
			break
		except:
			print("Not a valid option")
			continue
	s.sendall(bytearray(uid + [4, 1, 1, toSend]))
	print("data sent!!")
	return toSend


# Starting connection:
def connect_to_server(s):
	global AUTO_PORT
	global AUTO_HOST
	if (AUTO_HOST != "" and AUTO_PORT != 0):
		try:
			s.connect((AUTO_HOST, AUTO_PORT))
			return
		except:
			print("Can't connect to " + "http://" + AUTO_HOST + ":" + str(AUTO_PORT))
	no_connection = True
	while no_connection:
		print("Enter empty field to use default values: 127.0.0.1:49152")
		host = input("Enter the server IP address [255.255.255.255]: ")
		host = HOST if len(host) == 0 else host

		port = -1
		while (port < 0 or port > 65535):

			port_input = input("Enter the server port number [0 - 65536]: ")
			
			if len(port_input) == 0:
				port = PORT
				break
			try:
				port = int(port_input)
			except:
				print(port_input + " was not a valid option")

		print("Making connection to port: ", port);

		address_string = "http://" + host + ":" + str(port)
		print(address_string)
		
		AUTO_HOST = host
		AUTO_PORT = port
		try:
			s.connect((host, port))
			return
		except:
			print("Can't connect to " + address_string)

if __name__ == "__main__":
    main()
