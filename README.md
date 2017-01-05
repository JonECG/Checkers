# Checkers
It's literally just CLI checkers. It has an AI and online capabilities

### Building the application:

I mostly developed the program on windows with Microsoft Visual Studio 2015 update 3 so using that will more accurately represent what I was seeing when developing it. However, I kept things cross-platform as per the instructions and have also tested it on Linux.

##### Windows (Tested on Windows 10 ver1607 x64)
* **Compiler: Microsoft Visual Studio 2015 update 3**
* Open solution at *./Checkers-JPearl/Checkers-JPearl.sln*
* Select the Release configuration and build solution
    * You can select *Debug*, but be warned that the terminal will be flooded with verbose debug messages regarding the networking and AI that are ```#ifdef```’d out in Release
    * The applications will be placed in *./Checkers-JPearl/Builds/Win32/Release* (Or Debug if you selected Debug)
        * Checkers-JPearl.exe is the server but can also act as a client while CheckersClient-JPearl.exe is a dedicated client

##### Linux (Tested on Ubuntu 16.04.1 x64)
* **Compiler: GCC 5.4.0 20160609**
* Open a terminal in the root directory of the cloned git repo
* ```$ cd ./Checkers-JPearl/```
* ```$ make```
    * This is essentially the release build represented by running VS if you want a debug build with symbols to attach gdb to or just want to see the same verbose logging from VS’ debug build call ``` $ make 'CXXFLAGS=-g -DDEBUG -std=c++11'``` and you can clean with ```$ make clean```
* The applications will be placed in *./Checkers-JPearl/Builds/Linux/*
    * Checkers-JPearl is the server but can also act as a client while CheckersClient-JPearl is a dedicated client

### Running the application:

Checkers-JPearl is able to run as a server and a client so you can do everything from this application.
* Playing a game locally
    * Select option 1 on starting and both players take input from the local machine
* Play an AI game
    * Select option 2 to play against an AI player or option 3 to watch two AI players duke it out
        * AI Difficulty Level is just another way of saying how many layers the AI recurses into possible board state. Where 0 is no recursion and the AI seeks instant gratification with wreckless abandon, and  towards 9 is where the AI may sacrifice pieces to set up moves for it in the short term. I run a pretty beefy rig and the AI can take around 1 minute to find a move on level 9 so I recommend playing around 5 if you want quicker play.
* Host a server
    * Select option 4 to start a server on this process and select a port you would like to listen to. This server is active until you stop the server by selecting option 4 or quit. Note: you can still play games while hosting a server and even connect as a client to your own or another’s server.
        * As with hosting any server – make sure to forward your ports, DMZ, or any preferred flavor of getting incoming traffic on that port to the respective device otherwise incoming connections from outside your local network will be rejected or dropped. Implementing NAT punchthrough is a bit out of scope.
* Play a game online (Can also be done from CheckersClient-JPearl)
    * Select option 5 and input the host’s address and port it is listening on to connect to a server.
    * From this point you can opt to play with another player online (which will wait until there is another player ready) or you can play against an AI that is being simulated on the server
    * While playing online you will notice a [YOU] marker on the input field if it is your turn to go.

### Playing checkers:

For the most part, I implemented the rules of American Checkers:
* 8x8 board with 3 rows of pieces starting from the end of the board for each player only on the diagonals of the board
* O goes first
* Men can only move forward diagonally to an adjacent and capture forward diagonally if there is an opposing piece in the adjacent square and the square just beyond it is free, and will put current piece there
* Kings can move in any diagonal to an square and can capture in any direction
* Men that reach the end of the board become kings
* If a jump is available, it must be taken
* If a jump has started, it must continue until there are no jumps available
* Pieces are not removed until the end of the turn and can potentially block later jumps (is implemented, but doesn’t really matter as it’s impossible to get into this situation without flying kings)
* Winning is determined by capturing all opponents pieces or having your opponent have no available moves on the start of their turn or having your opponent forfeit
* Drawing is determined by creating the same board state for the 4th time in a row

# Playing checkers in the application
* On every turn, the application will present the current board state and all of the moves possible to the current player.
* If it is your turn, enter a valid move to perform it. You can also forfeit by typing ‘forfeit’ as your move
* The game is played until there is a win or a draw
* Players in online games that disconnect will be treated as a forfeiting on their next turn
* On application end the following will be returned
    * -1 : No game was played completely
        * This can happen if a client is disconnected from a server or if you run the application without actually playing a game yourself (hosting a server and quitting)
    * 0 : The last game played completely resulted in a draw
    * 1 : The last game played completely resulted in a win for O
    * 2 : The last game played completely resulted in a win for X
