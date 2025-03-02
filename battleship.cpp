#include <iostream>
#include <cstring>
#include <thread> // Threading Library To Handle Threading 
#include <mutex> // Mutex Library To Apply Locks And Mutes 
#include <chrono> // //Chrono Library Calculate Execution Time And Work With Time
using namespace std;

// USER Player1 And AI Player2 
int player1 = 1; // Current USER Player 1 Number 
int player2 = 2; // Current AI Player 2 Number
char boardPlayer1[10][10]; // USER Player 1 Board 
char boardPlayer2[10][10]; // AI Player 2 Board 

// Battleship Board States
const char WATER = '~';
const char SHIP = 'S';
const char HIT = 'X';
const char MISS = 'O';

// Battleship Pieces 
// First Index Ships X Coordinate 
// Second Index Ships Y Coordinate
// Third Index Current Player 

// Player Battle Ships
int carrier1[5][2][2]; // USER Carrier Ship - 5 Holes
int battleship1[4][2][2]; // USER Battleship Ship - 4 Holes
int cruiser1[3][2][2]; // USER Cruiser Ship - 3 Holes
int submarine1[3][2][2]; // USER Submarine Ship - 3 Holes
int destroyer1[1][2][2]; // USER Destroyer Ship - 2 Holes

// AI Battle Ships
int carrier2[5][2][2]; // AI Carrier Ship -4 Holes
int battleship2[4][2][2]; // AI Battleship Ship - 4 Holes
int cruiser2[3][2][2]; // AI Cruiser Ship - 3 Holes
int submarine2[3][2][2]; // AI Submarine Ship - 3 Holes
int destroyer2[1][2][2]; // AI Destroyer Ship - 2 Holes

void initializeBoard(char playerBoard[10][10]); // Initialize Player # Battleship Board
void printBoard(char board[10][10], int playerNumber); // Print Battleship Board
void monteCarloTreeSearch(char playerBoard[10][10]); // Monte Carlo Tree Search 

int main() {
    //initializeBoard(boardPlayer1); // Initialize All Empty Spaces From Player 1 Board To Water Spaces '~'
    //initializeBoard(boardPlayer2); // Initialize All Empty Spaces From Player 2 Board To Water Spaces '~'

    char boardPlayer1[10][10] = {
        {'~', '~', '~', '~', '~', '~', '~', 'X', 'X', '~'},
        {'~', '~', '~', '~', '~', '~', '~', '~', '~', '~'},
        {'~', '~', '~', '~', '~', '~', '~', '~', '~', '~'},
        {'~', '~', 'X', 'X', 'X', 'X', 'X', '~', '~', '~'},
        {'~', '~', '~', '~', '~', '~', '~', '~', '~', '~'},
        {'~', '~', '~', '~', '~', '~', 'X', '~', '~', '~'},
        {'~', 'X', '~', '~', '~', '~', 'X', '~', '~', '~'},
        {'~', 'X', '~', '~', '~', '~', 'X', '~', '~', 'X'},
        {'~', 'X', '~', '~', '~', '~', 'X', '~', '~', 'X'},
        {'~', '~', '~', '~', '~', '~', '~', '~', '~', 'X'}
    };

    printBoard(boardPlayer1, player1); // Print Player 1 Board 
    //printBoard(boardPlayer2, player2); // Print Player 2 Board 

    return 0;
}

// Initialize Player # Battleship Board
void initializeBoard(char playerBoard[10][10]) {
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            playerBoard[i][j] = WATER;
        }
    }
} 

// Print Battleship Board
void printBoard(char board[10][10], int playerNumber) {
    int number = 49;
    int letter = 65; 

    cout << "\t Player " <<  playerNumber << " Board"  << endl << endl;
    
    cout << " ";

    for (int i = 0; i < 10; i++) {
        cout << "  " << (char)(letter + i);
    }

    cout << endl;
    
    for (int i = 0; i < 10; i++) {
        
        if(i < 9) {
            cout << (char)(number + i) << "  ";
        } else {
            cout << "10" << " ";
        }

        for (int j = 0; j < 10; j++) {
            cout << board[i][j] << "  ";
        }
        cout << endl;
    }

    cout << endl << endl;
}

// Monte Carlo Tree Search
void monteCarloTreeSearch(char playerBoard[10][10]) {

}  