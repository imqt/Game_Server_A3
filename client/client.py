import socket
import sys
from os import system, name 

HOST = "127.0.0.1"
PORT = 49152

AUTO_HOST = "";
AUTO_PORT = 0;

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

# UI
def clear_screen():   
    # for windows 
    if name == 'nt': 
        _ = system('cls') 
  
    # for mac and linux(here, os.name is 'posix') 
    else: 
        _ = system('clear') 

def display_game_screen(board):
	# clear_screen()
	print("Guide board: ", end="")
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

def update_board(board, moves_list):
	for index in range(len(moves_list)):
		board[moves_list[index]] = "O" if index%2 else "X"


# Communication
def recv_byte(s):
	byte_recv = s.recv(1)
	converted_data = ord(str(byte_recv, "utf-8")[0]) # get ascii val of byte_recv as an int
	return converted_data

def recv_moves_list(s):
	moves_list = []
	data = recv_byte(s)
	print("Received data: ", data)
	while data != 9:
		moves_list.append(data)
		data = recv_byte(s)
		print("Received data: ", data)

	print("Moves: ", moves_list)
	return moves_list

def recv_game_end(s, play_turn):
	print("You are playing:", "O" if play_turn else "X")
	other_player = ("X" if play_turn else "O")
	game_state = recv_byte(s)
	if game_state == 0:
		print("Game in progress!")
		return 0
	elif game_state == play_turn + 1:
		print("\nYou won against " + other_player + "!\n")	
	elif game_state == 3:
		print("\nTie game!\n")
	else:
		print("\nYou lost to " + other_player + "!\n")

	return game_state

def prompt_send_data(s):
	while 1:
		toSend = input("\n[Make your move]: ")
		if toSend == "end":
			system(exit)
		try:
			toSend = int(toSend)
		except:
			print("Not a valid option")
			continue
		break
	s.sendall(bytes(chr(toSend), 'utf-8'))


# TTT gameplay
def ttt(s):
	print("TicTacToe")
	# Initialize board
	board = ["-" for x in range(9)]
	display_game_screen(board)
	print("Waiting for server!")

	# First move
	first_move_data = recv_moves_list(s)
	print(first_move_data)
	update_board(board, first_move_data)
	display_game_screen(board)
	game_state = True
	play_turn = len(first_move_data) # 0 => X , 1 ==> O
	if recv_game_end(s, play_turn) != 0:
		game_state = False
	
	if game_state:
		prompt_send_data(s)

	# Game loop
	while game_state:
		moves_list = recv_moves_list(s)
		update_board(board, moves_list)
		display_game_screen(board)
		if recv_game_end(s, play_turn) != 0:
			game_state = False
			break

		if (play_turn == len(moves_list)%2):
			prompt_send_data(s)
		else:
			print("Waiting for " + ("X" if play_turn else "O") + " player!")

	s.close()
	print("[ PRESS ENTER TO PLAY AGAIN! ]")
	print("OR\n[ Enter 'quit' to exit the game! ]")
	if (input("[Command]:") == "quit"):
		return False
	else:
		return True

def recv_rps_game_status():
	return 0

# RPS gameplay
def rps(s):
	print("RPS")
	prompt_send_data(s)
	s.close()
	print("[ PRESS ENTER TO PLAY AGAIN! ]")
	print("OR\n[ Enter 'quit' to exit the game! ]")
	if (input("[Command]:") == "quit"):
		return False
	else:
		return True


def choose_game():
	game_choice = ""
	while 1:
		game_choice = input("Choose game:\n(1) TicTacToe\n(2) RockPaperScissors\n[ Choice ]:")
		try:
			game_choice = int(game_choice)
		except:
			print("Not a valid option")
			continue
		break
	return game_choice

def main():

	while 1:
		game_choice = choose_game()

		with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
			connect_to_server(s)
			s.sendall(bytes(chr(game_choice), "utf-8"))

			if game_choice == 1:
				if not ttt(s):
					return;
			elif game_choice == 2:
				if not rps(s):
					return;


if __name__ == "__main__":
    main()
 

