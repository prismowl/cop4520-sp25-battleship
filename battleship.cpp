#include <iostream>
#include <cstring>
using namespace std;

int player1 = 1;
int player2 = 2;
char boardPlayer1[10][10];
char boardPlayer2[10][10];

// Print Battleship Board
void printBoard(char board[10][10], int playerNumber);

int main() {

    memset(boardPlayer1, '~', sizeof(boardPlayer1));
    memset(boardPlayer2, '~', sizeof(boardPlayer2));

    printBoard(boardPlayer1, player1);

    printBoard(boardPlayer2, player2);


    return 0;
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