#include <iostream>
#include <cstring>
#include <vector>
#include <cmath>       // For math calculation handling
#include <random>      // For random number generation
#include <thread>      // Threading library to handle threading 
#include <mutex>       // Mutex library to apply locks and mutes 
#include <chrono>      // Chrono library to calculate execution time
#include <limits>      // For numeric_limits

using namespace std;

const int BOARD_SIZE = 10;    // Board dimensions (rows and columns)
const int NUM_THREADS = 4;    // Number of threads for parallel execution
mutex mtx;                    // Mutex for synchronizing shared data

// USER Player1 
int player1 = 1;                              // Current USER Player 1 number 
char boardPlayer1[BOARD_SIZE][BOARD_SIZE];      // USER Player 1 Board 

// AI Player2 
int player2 = 2;                              // Current AI Player 2 number
char boardPlayer2[BOARD_SIZE][BOARD_SIZE];      // AI Player 2 Board 

int moveAmount = 0;                           // Amount of moves to win the game

// Battleship Board States
const char WATER = '~';
const char SHIP = 'S';
const char HIT = 'X';
const char MISS = 'O';

// Battleship Pieces (for future use)
int carrier1[5][2][2];      // USER Carrier Ship - 5 Holes
int battleship1[4][2][2];   // USER Battleship Ship - 4 Holes
int cruiser1[3][2][2];      // USER Cruiser Ship - 3 Holes
int submarine1[3][2][2];    // USER Submarine Ship - 3 Holes
int destroyer1[1][2][2];    // USER Destroyer Ship - 2 Holes
bool huntMode1 = false;

int carrier2[5][2][2];      // AI Carrier Ship - 4 Holes
int battleship2[4][2][2];   // AI Battleship Ship - 4 Holes
int cruiser2[3][2][2];      // AI Cruiser Ship - 3 Holes
int submarine2[3][2][2];    // AI Submarine Ship - 3 Holes
int destroyer2[1][2][2];    // AI Destroyer Ship - 2 Holes
bool huntMode2 = false;

// A Node in the Monte Carlo Search Tree
struct MCTSNode {
    double wins = 0.0;         // How many "wins" from playout
    int visits = 0;            // How many times this node has been visited
    int moveRow = -1;          // Row of the move leading to this node
    int moveCol = -1;          // Column of the move leading to this node
    bool terminal = false;     // Is the current state a winning game?
    char boardState[BOARD_SIZE][BOARD_SIZE];  // Board configuration for this node
    vector<MCTSNode*> children;               // Children nodes
    MCTSNode* parent = nullptr;               // Parent node
};

int MCTS_ITERATIONS = 200; // Number of iterations to run the MCTS

//// Function Declarations ////

// Board Functions 
void initializeBoard(char playerBoard[BOARD_SIZE][BOARD_SIZE]);  // Initialize board with WATER
void printBoard(char board[BOARD_SIZE][BOARD_SIZE], int playerNumber); // Print board

// Monte Carlo Search Tree Functions 
void monteCarloTreeSearch(char playerBoard[BOARD_SIZE][BOARD_SIZE]); // Run the MCTS
MCTSNode* runMCTSPhases(MCTSNode* root); // Execute selection, expansion, simulation, backpropagation

// MCTS Phases
MCTSNode* selection(MCTSNode* node); // Select a child by its UCB score
void expansion(MCTSNode* node);      // Expand the current node by adding children
double simulation(MCTSNode* node);     // Simulate a random playout from current node
void backpropagation(MCTSNode* node, double result); // Update the tree with simulation result

// Helper Functions
bool isGameOver(const char board[BOARD_SIZE][BOARD_SIZE]);  // Check if any ship remains
vector<pair<int, int>> getPossibleMoves(const char board[BOARD_SIZE][BOARD_SIZE]); // Get all legal moves
bool applyMove(char board[BOARD_SIZE][BOARD_SIZE], int row, int col); // Apply a move to the board
void copyBoard(const char source[BOARD_SIZE][BOARD_SIZE], char dest[BOARD_SIZE][BOARD_SIZE]); // Copy board state
void deleteTree(MCTSNode* root);  // Clean up tree memory
double UCB(MCTSNode* node);       // Calculate UCB score for node

// Parallel simulation and extra functions
void parallelSimulation(vector<MCTSNode*>& nodes);
bool bombed(const char board[BOARD_SIZE][BOARD_SIZE], int row, int col); // Check if the space was bombed
void huntAndTarget(char board[BOARD_SIZE][BOARD_SIZE], int row, int col); // Lock on to a HIT space and scan around it

//// Function Definitions ////

// Initialize the board with WATER characters
void initializeBoard(char playerBoard[BOARD_SIZE][BOARD_SIZE]) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            playerBoard[i][j] = WATER;
        }
    }
}

// Print the board with row and column labels
void printBoard(const char board[BOARD_SIZE][BOARD_SIZE]) {
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


// Run multiple simulations in parallel
void parallelSimulation(vector<MCTSNode*>& nodes) {
    vector<thread> threads;
    vector<double> results(nodes.size());

    // Launch a thread for each node simulation
    for (size_t i = 0; i < nodes.size(); ++i) {
        threads.emplace_back([&, i] {
            results[i] = simulation(nodes[i]);
        });
    }

    // Join all threads to ensure completion
    for (auto& t : threads) {
        t.join();
    }

    // Backpropagate simulation results
    for (size_t i = 0; i < nodes.size(); ++i) {
        backpropagation(nodes[i], results[i]);
    }
}

// Main MCTS routine: search until game is over
void monteCarloTreeSearch(char playerBoard[BOARD_SIZE][BOARD_SIZE]) {
    while (!isGameOver(playerBoard)) {
        printBoard(playerBoard);
        // Create root node with current board state
        MCTSNode* root = new MCTSNode();
        copyBoard(playerBoard, root->boardState);
        root->terminal = isGameOver(root->boardState);

        if (root->terminal) {
            delete root;
            break;
        }

        // Expand root to create children nodes
        expansion(root);

        // Run MCTS iterations to choose the best move
        MCTSNode* bestChild = runMCTSPhases(root);

        if (!bestChild) {
            deleteTree(root);
            break;
        }

        // Apply the chosen move
        applyMove(playerBoard, bestChild->moveRow, bestChild->moveCol);

        // Clean up the tree
        deleteTree(root);

        printBoard(playerBoard); // Debug: show current board

        moveAmount++; // Increment move counter
    }
}

// Run MCTS iterations (selection, expansion, simulation, backpropagation)
MCTSNode* runMCTSPhases(MCTSNode* root) {
    for (int i = 0; i < MCTS_ITERATIONS; i++) {
        MCTSNode* current = root;

        // Selection phase: traverse to a promising node
        while (!current->children.empty() && !current->terminal) {
            current = selection(current);
        }

        // Expansion phase: if node is not terminal, expand it
        if (!current->terminal) {
            if (current->children.empty()) {
                expansion(current);
            }
            if (!current->children.empty()) {
                static mt19937 rng(random_device{}());
                uniform_int_distribution<int> dist(0, (int)current->children.size() - 1);
                current = current->children[dist(rng)];
            }
        }

        // Simulation phase: run simulations in parallel on children
        parallelSimulation(current->children);
    }

    double bestRate = -1.0;
    MCTSNode* bestChild = nullptr;
    for (auto* child : root->children) {
        if (child->visits > 0) {
            double rate = child->wins / (double)child->visits;
            if (rate > bestRate) {
                bestRate = rate;
                bestChild = child;
            }
        }
    }
    return bestChild;
}

// Selection: choose the child with the highest UCB score
MCTSNode* selection(MCTSNode* node) {
    MCTSNode* bestChild = nullptr;
    double bestValue = -1e9;
    for (auto* child : node->children) {
        double ucbValue = UCB(child);
        if (ucbValue > bestValue) {
            bestValue = ucbValue;
            bestChild = child;
        }
    }
    return bestChild;
}

// Expansion: generate a child for every possible move
void expansion(MCTSNode* node) {
    if (node->terminal) return;
    vector<pair<int, int>> moves = getPossibleMoves(node->boardState);
    for (auto& m : moves) {
        int r = m.first;
        int c = m.second;
        MCTSNode* child = new MCTSNode();
        child->parent = node;
        copyBoard(node->boardState, child->boardState);
        applyMove(child->boardState, r, c);
        child->moveRow = r;
        child->moveCol = c;
        child->terminal = isGameOver(child->boardState);
        node->children.push_back(child);
    }
}

// Simulation: simulate a random playout from the node's board state
double simulation(MCTSNode* node) {
    char simBoard[BOARD_SIZE][BOARD_SIZE];
    copyBoard(node->boardState, simBoard);
    if (isGameOver(simBoard)) {
        return 1.0;
    }
    // Each thread gets its own random engine instance
    thread_local mt19937 rng((unsigned)random_device{}());
    int moveLimit = 100;
    for (int i = 0; i < moveLimit; i++) {
        if (isGameOver(simBoard)) {
            return 1.0;
        }
        auto possible = getPossibleMoves(simBoard);
        if (possible.empty()) {
            break;
        }
        uniform_int_distribution<int> dist(0, (int)possible.size() - 1);
        int idx = dist(rng);
        int rr = possible[idx].first;
        int cc = possible[idx].second;
        applyMove(simBoard, rr, cc);
    }
    return isGameOver(simBoard) ? 1.0 : 0.0;
}

// Backpropagation: update nodes with simulation result
void backpropagation(MCTSNode* node, double result) {
    MCTSNode* current = node;
    while (current != nullptr) {
        current->visits++;
        current->wins += result;
        current = current->parent;
    }
}

// Check if any ship ('S') remains on the board
bool isGameOver(const char board[BOARD_SIZE][BOARD_SIZE]) {
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (board[r][c] == SHIP) return false;
        }
    }
    return true;
}

// Get all possible moves: cells not yet bombed (not HIT or MISS)
vector<pair<int, int>> getPossibleMoves(const char board[BOARD_SIZE][BOARD_SIZE]) {
    vector<pair<int, int>> moves;
    for (int r = 0; r < BOARD_SIZE; ++r)
        for (int c = 0; c < BOARD_SIZE; ++c)
            //direct comparison here. Calling bombed was creating overhead slowing run
            if (board[r][c] != HIT && board[r][c] != MISS)
                moves.emplace_back(r, c);
    return moves;
}

// Apply a move: mark HIT if ship exists, else mark MISS
bool applyMove(char board[BOARD_SIZE][BOARD_SIZE], int row, int col) {
    //this way board is only evalutated once.
    //attempt for better formance
    bool hit = board[row][col] == SHIP;
    if (hit) {
        board[row][col] = HIT;
    } else {
        board[row][col] = MISS;
    }
    return hit;
}


// Copy the board state from source to destination
void copyBoard(const char src[BOARD_SIZE][BOARD_SIZE], char dest[BOARD_SIZE][BOARD_SIZE]) {
    memcpy(dest, src, sizeof(char) * BOARD_SIZE * BOARD_SIZE);
}


// Delete the MCTS tree to free memory
void deleteTree(MCTSNode* root) {
    for (MCTSNode* child : root->children) {
        deleteTree(child);
    }
    delete root;
}

// Calculate the UCB score for a node
double UCB(MCTSNode* node) {
    if (node->visits == 0) return numeric_limits<double>::infinity();
    double exploitation = node->wins / (double)node->visits;
    double exploration = sqrt(2.0 * log((double)node->parent->visits) / (double)node->visits);
    return exploitation + exploration;
}

// // Check if the given cell has already been bombed
// bool bombed(char board[BOARD_SIZE][BOARD_SIZE], int row, int col) {
//     return (board[row][col] == HIT || board[row][col] == MISS);
// }

// void huntAndTarget(char board[BOARD_SIZE][BOARD_SIZE], int row, int col) {
//     if (row > 0 && !bombed(board, row - 1, col) && applyMove(board, row - 1, col))
//         huntAndTarget(board, row - 1, col);
//     if (row < BOARD_SIZE - 1 && !bombed(board, row + 1, col) && applyMove(board, row + 1, col))
//         huntAndTarget(board, row + 1, col);
//     if (col > 0 && !bombed(board, row, col - 1) && applyMove(board, row, col - 1))
//         huntAndTarget(board, row, col - 1);
//     if (col < BOARD_SIZE - 1 && !bombed(board, row, col + 1) && applyMove(board, row, col + 1))
//         huntAndTarget(board, row, col + 1);
//     return;
// }

int main() {
    char board[BOARD_SIZE][BOARD_SIZE] = {
        {'~', 'S', '~', '~', '~', '~', '~', '~', '~', '~'},
        {'~', 'S', '~', '~', '~', '~', '~', '~', '~', '~'},
        {'~', 'S', '~', 'S', 'S', 'S', 'S', '~', '~', '~'},
        {'~', 'S', '~', '~', '~', '~', '~', '~', '~', '~'},
        {'~', 'S', '~', 'S', '~', '~', 'S', 'S', 'S', '~'},
        {'~', '~', '~', 'S', '~', '~', '~', '~', '~', '~'},
        {'~', '~', '~', 'S', '~', '~', '~', '~', '~', '~'},
        {'~', '~', '~', '~', '~', '~', '~', '~', '~', '~'},
        {'~', '~', '~', '~', '~', '~', '~', 'S', 'S', '~'},
        {'~', '~', '~', '~', '~', '~', '~', '~', '~', '~'}
    };

    auto start = chrono::high_resolution_clock::now();
    monteCarloTreeSearch(board);
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);

    cout << "Game completed in " << moveAmount << " AI moves." << endl;
    cout << "Time: " << duration.count() << " ms" << endl;

    return 0;
}