#include <iostream>
#include <cstring>
#include <vector>
#include <cmath>
#include <random>
#include <thread>
#include <mutex>
#include <chrono>
#include <limits>
#include <future>
#include <algorithm>

using namespace std;

const int BOARD_SIZE = 10;
const int NUM_THREADS = 4;
mutex mtx;

int player1 = 1;
char boardPlayer1[BOARD_SIZE][BOARD_SIZE];  // User board
int player2 = 2;
char boardPlayer2[BOARD_SIZE][BOARD_SIZE];  // AI board

int moveAmount = 0;
const char WATER = '~', SHIP = 'S', HIT = 'X', MISS = 'O';
const int MCTS_ITERATIONS = 100;
const int SIMULATION_COUNT = 32;
vector<pair<int, int>> aiTargets; // Stores spots to target when AI hits a ship

struct MCTSNode {
    double wins = 0.0;
    int visits = 0;
    int moveRow = -1;
    int moveCol = -1;
    bool terminal = false;
    char boardState[BOARD_SIZE][BOARD_SIZE];
    vector<MCTSNode*> children;
    MCTSNode* parent = nullptr;
};

void printBoard(char board[BOARD_SIZE][BOARD_SIZE]);
void monteCarloTreeSearch(char playerBoard[BOARD_SIZE][BOARD_SIZE]);
MCTSNode* runMCTSPhases(char board[BOARD_SIZE][BOARD_SIZE]);
MCTSNode* selection(MCTSNode* node);
void expansion(MCTSNode* node);
double simulation(MCTSNode* node);
void backpropagation(MCTSNode* node, double result);
bool isGameOver(char board[BOARD_SIZE][BOARD_SIZE]);
vector<pair<int, int>> getPossibleMoves(char board[BOARD_SIZE][BOARD_SIZE]);
bool applyMove(char board[BOARD_SIZE][BOARD_SIZE], int row, int col);
void copyBoard(char source[BOARD_SIZE][BOARD_SIZE], char dest[BOARD_SIZE][BOARD_SIZE]);
void deleteTree(MCTSNode* root);
double UCB(MCTSNode* node);
void playerTurn();
void aiTurn();
bool isValidMove(char board[BOARD_SIZE][BOARD_SIZE], int row, int col);

void printBoard(char board[BOARD_SIZE][BOARD_SIZE]) {
    cout << "\n   ";
    for (int c = 0; c < BOARD_SIZE; ++c) cout << (char)('A' + c) << ' ';
    cout << '\n';
    for (int r = 0; r < BOARD_SIZE; ++r) {
        cout << (r + 1 < 10 ? " " : "") << r + 1 << ' ';
        for (int c = 0; c < BOARD_SIZE; ++c) cout << board[r][c] << ' ';
        cout << '\n';
    }
    cout << endl;
}

void copyBoard(char src[BOARD_SIZE][BOARD_SIZE], char dest[BOARD_SIZE][BOARD_SIZE]) {
    memcpy(dest, src, sizeof(char) * BOARD_SIZE * BOARD_SIZE);
}

bool isGameOver(char board[BOARD_SIZE][BOARD_SIZE]) {
    for (int r = 0; r < BOARD_SIZE; ++r) {
        for (int c = 0; c < BOARD_SIZE; ++c) {
            if (board[r][c] == SHIP) return false;
        }
    }
    return true;
}

bool isValidMove(char board[BOARD_SIZE][BOARD_SIZE], int row, int col) {
    return row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE &&
        (board[row][col] == WATER || board[row][col] == SHIP);
}


// Apply a move to the board. Return false if invalid, true if successful.
bool applyMove(char board[BOARD_SIZE][BOARD_SIZE], int row, int col) {
    if (!isValidMove(board, row, col)) return false;

    if (board[row][col] == SHIP) {
        board[row][col] = HIT;

        // Add adjacent tiles for targeting
        vector<pair<int, int>> directions = { {1,0}, {-1,0}, {0,1}, {0,-1} };
        for (auto [dr, dc] : directions) {
            int newRow = row + dr, newCol = col + dc;
            if (isValidMove(board, newRow, newCol)) {
                aiTargets.emplace_back(newRow, newCol);
            }
        }
    }
    else {
        board[row][col] = MISS;
    }

    return true;
}


void playerTurn() {
    while (true) {
        string input;
        cout << "Your turn. Enter your move (e.g., A1): ";
        cin >> input;

        // Check if the input length is correct (A1 to A10)
        if (input.length() < 2 || input.length() > 3) {
            cout << "Invalid input format. Please try again.\n";
            continue;
        }

        // Parse column (the letter part, e.g., 'A')
        int col = toupper(input[0]) - 'A';  // 'A' -> 0, 'B' -> 1, etc.
        if (col < 0 || col >= BOARD_SIZE) {
            cout << "Invalid column. Please try again.\n";
            continue;
        }

        // Parse row (the number part, e.g., '1', '10')
        int row;
        try {
            row = stoi(input.substr(1)) - 1;  // Convert row to 0-indexed
        }
        catch (...) {
            cout << "Invalid row. Please try again.\n";
            continue;
        }

        if (row < 0 || row >= BOARD_SIZE) {
            cout << "Invalid row. Please try again.\n";
            continue;
        }

        // Check if the move is valid on the AI's board
        if (!isValidMove(boardPlayer2, row, col)) {
            cout << "Invalid move. Please try again.\n";
        }
        else {
            applyMove(boardPlayer2, row, col);  // Apply the move
            printBoard(boardPlayer2);           // Display the updated AI board
            break;                              // Exit the loop after a valid move
        }
    }
}

void aiTurn() {
    cout << "AI is thinking...\n";

    vector<pair<int, int>> moves = getPossibleMoves(boardPlayer1);
    if (moves.empty()) return; // No moves left

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> dist(0, moves.size() - 1);
    auto [row, col] = moves[dist(gen)];

    cout << "AI chose: " << char('A' + col) << row + 1 << endl;
    applyMove(boardPlayer1, row, col);
    printBoard(boardPlayer1);

    // If AI sinks the ship, reset target mode
    if (isGameOver(boardPlayer1) || aiTargets.empty()) {
        aiTargets.clear();
    }
}

vector<pair<int, int>> getPossibleMoves(char board[BOARD_SIZE][BOARD_SIZE]) {
    vector<pair<int, int>> moves;

    // If there are targets from a previous hit, focus on them first
    if (!aiTargets.empty()) {
        moves = aiTargets;
        return moves; // Prioritize targeting adjacent moves
    }

    // Otherwise, pick from all valid spots (hunt mode)
    for (int r = 0; r < BOARD_SIZE; ++r) {
        for (int c = 0; c < BOARD_SIZE; ++c) {
            if (board[r][c] == WATER || board[r][c] == SHIP) {
                moves.emplace_back(r, c);
            }
        }
    }

    return moves;
}



double UCB(MCTSNode* node) {
    if (node->visits == 0) return numeric_limits<double>::max();
    return node->wins / node->visits + sqrt(2 * log(node->parent->visits) / node->visits);
}

MCTSNode* selection(MCTSNode* node) {
    while (!node->children.empty()) {
        MCTSNode* best = nullptr;
        double max_UCB = -numeric_limits<double>::max();
        for (MCTSNode* child : node->children) {
            double ucb_value = UCB(child);
            if (ucb_value > max_UCB) {
                max_UCB = ucb_value;
                best = child;
            }
        }
        node = best;
    }
    return node;
}

void expansion(MCTSNode* node) {
    vector<pair<int, int>> moves = getPossibleMoves(node->boardState);
    for (auto& move : moves) {
        MCTSNode* child = new MCTSNode();
        child->parent = node;
        child->moveRow = move.first;
        child->moveCol = move.second;
        node->children.push_back(child);
        copyBoard(node->boardState, child->boardState);
        applyMove(child->boardState, child->moveRow, child->moveCol);
        child->terminal = isGameOver(child->boardState);
    }
}

double simulation(MCTSNode* node) {
    vector<pair<int, int>> possible_moves = getPossibleMoves(node->boardState);
    shuffle(possible_moves.begin(), possible_moves.end(), mt19937(random_device()()));
    char simu_board[BOARD_SIZE][BOARD_SIZE];
    copyBoard(node->boardState, simu_board);

    for (int step = 0; step < 30 && !isGameOver(simu_board); ++step) {
        if (possible_moves.empty()) break;
        auto move = possible_moves.back();
        possible_moves.pop_back();
        applyMove(simu_board, move.first, move.second);
    }
    return isGameOver(simu_board) ? 1.0 : 0.0;
}

void backpropagation(MCTSNode* node, double result) {
    while (node != nullptr) {
        node->visits++;
        node->wins += result;
        node = node->parent;
    }
}

MCTSNode* runMCTSPhases(char board[BOARD_SIZE][BOARD_SIZE]) {
    MCTSNode* root = new MCTSNode();
    copyBoard(board, root->boardState);

    for (int i = 0; i < MCTS_ITERATIONS; ++i) {
        MCTSNode* node = selection(root);
        if (!node->terminal) expansion(node);
        MCTSNode* leaf = (node->children.empty()) ? node : node->children[0];
        double result = simulation(leaf);
        backpropagation(leaf, result);
    }

    MCTSNode* bestMove = nullptr;
    double bestWinRate = -numeric_limits<double>::max();
    for (MCTSNode* child : root->children) {
        double winRate = child->wins / child->visits;
        if (winRate > bestWinRate) {
            bestWinRate = winRate;
            bestMove = child;
        }
    }

    return bestMove;
}

void deleteTree(MCTSNode* root) {
    if (!root) return;
    for (MCTSNode* child : root->children) {
        deleteTree(child);
    }
    delete root;
}

int main() {
    // Initialize boards with WATER and randomly place ships
    memset(boardPlayer1, WATER, sizeof(boardPlayer1));
    memset(boardPlayer2, WATER, sizeof(boardPlayer2));

    // Example of random ship placement for demonstration (replace with actual logic)
    boardPlayer1[0][0] = SHIP;  // Add ships for Player 1 (user)
    boardPlayer1[1][0] = SHIP;
    boardPlayer1[2][0] = SHIP;
    boardPlayer1[3][0] = SHIP;
    boardPlayer2[0][0] = SHIP;  // Add ships for Player 2 (AI)

    cout << "Welcome to Battleship!\n";

    while (!isGameOver(boardPlayer1) && !isGameOver(boardPlayer2)) {
        cout << "AI's Board (your target):\n";
        printBoard(boardPlayer2);
        playerTurn();  // Player 1's turn

        if (isGameOver(boardPlayer2)) {
            cout << "Congratulations! You won!\n";
            break;
        }

        aiTurn();  // AI's turn

        if (isGameOver(boardPlayer1)) {
            cout << "AI won! Better luck next time.\n";
            break;
        }
    }

    return 0;
}
