#include <iostream>
#include <cstring>
#include <vector>
#include <cmath>       // For math calculation handling
#include <random>      // For random number generation
#include <thread>      // Threading library to handle threading 
#include <mutex>       // Mutex library to apply locks and mutes 
#include <chrono>      // Chrono library to calculate execution time
#include <limits>      // For numeric_limits
#include <future>
#include <algorithm>

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
const char WATER = '~', SHIP = 'S', HIT = 'X', MISS = 'O';
const int MCTS_ITERATIONS = 100;
const int SIMULATION_COUNT = 32;

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

//// Function Declarations ////

// Board Functions 
void printBoard(char board[BOARD_SIZE][BOARD_SIZE], int playerNumber); // Print board

// Monte Carlo Search Tree Functions 
void monteCarloTreeSearch(char playerBoard[BOARD_SIZE][BOARD_SIZE]); // Run the MCTS
MCTSNode* runMCTSPhases(char board[BOARD_SIZE][BOARD_SIZE]); // Execute selection, expansion, simulation, backpropagation

// MCTS Phases
MCTSNode* selection(MCTSNode* node); // Select a child by its UCB score
void expansion(MCTSNode* node);      // Expand the current node by adding children
double simulation(MCTSNode* node);     // Simulate a random playout from current node
void backpropagation(MCTSNode* node, double result); // Update the tree with simulation result

// Helper Functions
bool isGameOver(char board[BOARD_SIZE][BOARD_SIZE]);  // Check if any ship remains
vector<pair<int, int>> getPossibleMoves(char board[BOARD_SIZE][BOARD_SIZE]); // Get all legal moves
bool applyMove(char board[BOARD_SIZE][BOARD_SIZE], int row, int col); // Apply a move to the board
void copyBoard(char source[BOARD_SIZE][BOARD_SIZE], char dest[BOARD_SIZE][BOARD_SIZE]); // Copy board state
void deleteTree(MCTSNode* root);  // Clean up tree memory
double UCB(MCTSNode* node);       // Calculate UCB score for node

// Parallel simulation and extra functions
void parallelSimulation(vector<MCTSNode*>& nodes);
bool bombed(char board[BOARD_SIZE][BOARD_SIZE], int row, int col); // Check if the space was bombed
void huntAndTarget(char board[BOARD_SIZE][BOARD_SIZE], int row, int col); // Lock on to a HIT space and scan around it

//// Function Definitions ////
// Print the board with row and column labels
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

// Copy the board state from source to destination
void copyBoard(char src[BOARD_SIZE][BOARD_SIZE], char dest[BOARD_SIZE][BOARD_SIZE]) {
    memcpy(dest, src, sizeof(char) * BOARD_SIZE * BOARD_SIZE);
}

// Main MCTS routine: search until game is over
void monteCarloTreeSearch(char board[BOARD_SIZE][BOARD_SIZE]) {
    while (!isGameOver(board)) {
        printBoard(board);
        if (isGameOver(board)) break;
        cout << "\nAI's turn...\n";
        MCTSNode* bestMove = runMCTSPhases(board);
        if (!bestMove) break;
        applyMove(board, bestMove->moveRow, bestMove->moveCol);
        moveAmount++;
        deleteTree(bestMove);
    }
    printBoard(board);
}

double simulation(MCTSNode* node) {
    //furutues vector for async iterations
    vector<future<double>> future_vect;
    
    //run simulations
    for (int i = 0; i < SIMULATION_COUNT; ++i) {
        future<double> future_simulation = async(launch::async, [node]() -> double {
            //make copy of board
            char simu_board[BOARD_SIZE][BOARD_SIZE];
            copyBoard(node->boardState, simu_board);
            
            //give each thread a random num instance
            static thread_local mt19937 rng(random_device{}());
            
            //2D vector for visted
            vector<vector<bool>> visited(BOARD_SIZE, vector<bool>(BOARD_SIZE, false));
            
            // store targets and hits
            vector<pair<int, int>> target_list;
            vector<pair<int, int>> hit_list;
            
            // attempt to hit targets near already hit part
            auto addDirectionalTargets = [ & ](int r1, int c1, int r2, int c2) {
                int delta_row = r2 - r1;
                int delta_col = c2 - c1;
                
                //head forwards
                int next_row = r2 + delta_row;
                int next_col = c2 + delta_col;
                while (next_row >= 0 && next_row < BOARD_SIZE &&
                       next_col >= 0 && next_col < BOARD_SIZE &&
                       !visited[next_row][next_col] &&
                       simu_board[next_row][next_col] != HIT &&
                       simu_board[next_row][next_col] != MISS)
                {
                    target_list.push_back(make_pair(next_row, next_col));
                    next_row += delta_row;
                    next_col += delta_col;
                }
                
                // head backwards
                next_row = r1 - delta_row;
                next_col = c1 - delta_col;
                while (next_row >= 0 && next_row < BOARD_SIZE &&
                       next_col >= 0 && next_col < BOARD_SIZE &&
                       !visited[next_row][next_col] &&
                       simu_board[next_row][next_col] != HIT &&
                       simu_board[next_row][next_col] != MISS)
                {
                    target_list.push_back(make_pair(next_row, next_col));
                    next_row -= delta_row;
                    next_col -= delta_col;
                }
            };
            
            //now for adjacent targets
            auto addAdjacentTargets = [ & ](int r, int c) {
                //possible offsets
                vector<pair<int, int>> neighbors = {
                    //up
                    make_pair(r - 1, c),
                    //down
                    make_pair(r + 1, c),
                    //left
                    make_pair(r, c - 1),
                    //right
                    make_pair(r, c + 1)
                };
                // add to target list
                for (auto& nbr : neighbors) {
                    int nr = nbr.first, nc = nbr.second;
                    if (nr >= 0 && nr < BOARD_SIZE &&
                        nc >= 0 && nc < BOARD_SIZE &&
                        !visited[nr][nc] &&
                        simu_board[nr][nc] != HIT &&
                        simu_board[nr][nc] != MISS)
                    {
                        target_list.push_back(make_pair(nr, nc));
                    }
                }
            };
            
            //run loop till game ends
            for (int step = 0; step < 30 && !isGameOver(simu_board); ++step) {
                pair<int, int> chosen_move;
                
                // hit priority targets
                if (!target_list.empty()) {
                    chosen_move = target_list.back();
                    target_list.pop_back();
                } else {
                    // gather all moves
                    vector<pair<int, int>> possible_moves = getPossibleMoves(simu_board);
                    if (possible_moves.empty()) {
                        break;
                    }
                    // Shuffle the moves to add randomness.
                    shuffle(possible_moves.begin(), possible_moves.end(), rng);
                    chosen_move = possible_moves[0];
                }
                
                int cur_row = chosen_move.first;
                int cur_col = chosen_move.second;
                
                // Skip already visited cells.
                if (visited[cur_row][cur_col]) {
                    continue;
                }
                visited[cur_row][cur_col] = true;
                
                // Check if the chosen cell is a ship.
                if (simu_board[cur_row][cur_col] == SHIP) {
                    simu_board[cur_row][cur_col] = HIT;
                    hit_list.push_back(make_pair(cur_row, cur_col));
                    
                    // Add adjacent cells to target list (simulate human targeting around a hit).
                    addAdjacentTargets(cur_row, cur_col);
                    
                    // If we have at least two hits, add directional targets.
                    if (hit_list.size() >= 2) {
                        int previousRow = hit_list[hit_list.size() - 2].first;
                        int previousCol = hit_list[hit_list.size() - 2].second;
                        int lastRow = hit_list[hit_list.size() - 1].first;
                        int lastCol = hit_list[hit_list.size() - 1].second;
                        if (previousRow == lastRow || previousCol == lastCol) {
                            addDirectionalTargets(previousRow, previousCol, lastRow, lastCol);
                        }
                    }
                } else {
                    // Mark as MISS if no ship is found.
                    simu_board[cur_row][cur_col] = MISS;
                }
            }
            
            // Return 1.0 if the board is in a game-over state (i.e. all ships sunk), else 0.0.
            return isGameOver(simu_board) ? 1.0 : 0.0;
        }); // End async lambda
        
        future_vect.push_back(move(future_simulation));
    }
    
    // Gather results from all simulation futures.
    double totalResult = 0.0;
    for (size_t i = 0; i < future_vect.size(); ++i) {
        totalResult += future_vect[i].get();
    }
    
    // Return the average outcome.
    return totalResult / SIMULATION_COUNT;
}





// Check if any ship ('S') remains on the board
bool isGameOver(char board[BOARD_SIZE][BOARD_SIZE]) {
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (board[r][c] == SHIP) return false;
        }
    }
    return true;
}



// Run multiple simulations in parallel
// void parallelSimulation(vector<MCTSNode*>& nodes) {
//     vector<thread> threads;
//     vector<double> results(nodes.size());

//     // Launch a thread for each node simulation
//     for (size_t i = 0; i < nodes.size(); ++i) {
//         threads.emplace_back([&, i] {
//             results[i] = simulation(nodes[i]);
//         });
//     }

//     // Join all threads to ensure completion
//     for (auto& t : threads) {
//         t.join();
//     }

//     // Backpropagate simulation results
//     for (size_t i = 0; i < nodes.size(); ++i) {
//         backpropagation(nodes[i], results[i]);
//     }
// }

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
    auto moves = getPossibleMoves(node->boardState);
    for (auto& m : moves) {
        MCTSNode* child = new MCTSNode();
        copyBoard(node->boardState, child->boardState);
        applyMove(child->boardState, m.first, m.second);
        child->moveRow = m.first;
        child->moveCol = m.second;
        child->terminal = isGameOver(child->boardState);
        child->parent = node;
        node->children.push_back(child);
    }
}

// Backpropagation: update nodes with simulation result
void backpropagation(MCTSNode* node, double result) {
    //continue iteration as long as theres nodes to go to
    while (node) {
        node->visits++;
        node->wins += result;
        //move up to pareent node
        node = node->parent;
    }
}


// Get all possible moves: cells not yet bombed (not HIT or MISS)
vector<pair<int, int>> getPossibleMoves(char board[BOARD_SIZE][BOARD_SIZE]) {
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

// Delete the MCTS tree to free memory
void deleteTree(MCTSNode* root) {
    for (auto* c : root->children) deleteTree(c);
    delete root;
}

// Run MCTS iterations (selection, expansion, simulation, backpropagation)
MCTSNode* runMCTSPhases(char board[BOARD_SIZE][BOARD_SIZE]) {

    //get all possible moves from the board that are not bombed yet
    vector<pair<int, int>> moves = getPossibleMoves(board);
    vector<MCTSNode*> possible_moves;

    //create nodes for each move in tree
    for (auto& m : moves) {
        //create new node
        MCTSNode* child = new MCTSNode();
        //copy current board state
        copyBoard(board, child->boardState);
        //applies move
        applyMove(child->boardState, m.first, m.second);
        //keep track of row and column index of the move
        child->moveRow = m.first;
        child->moveCol = m.second;
        //add move to list
        possible_moves.push_back(child);
    }

    //determine num of threads hardware can use and create vector
    const int num_threads = thread::hardware_concurrency();
    // num of iterations for each node
    const int num_possible_nodes = possible_moves.size();
    int iter_per_node = MCTS_ITERATIONS / num_possible_nodes;
    // reserve some threads to avoid reallocation mess
    vector<thread> threads;
    threads.reserve(num_threads);

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t, iter_per_node]() {
            // give thread random num instance
            static thread_local mt19937 rng(random_device{}());
            //distribute nodes to threads
            for (size_t i = t; i < possible_moves.size(); i += num_threads) {
                MCTSNode* child_node = possible_moves[i];
                //run num of iterations
                for (int iter = 0; iter < iter_per_node; ++iter) {
                    MCTSNode* current = child_node;
                    //expand node
                    expansion(current);
                    if (!current->children.empty()) {
                        //choose random move child
                        uniform_int_distribution<int> dist(0, (int)current->children.size() - 1);
                        current = current->children[dist(rng)];
                    }
                    //run simulation on current node
                    double result = simulation(current);
                    //update tree
                    backpropagation(current, result);    
                }
            }
        });
    }


    //join threads
    for (auto& t : threads) t.join();

    //find best node based on winrate
    MCTSNode* best = nullptr;
    double bestRate = -1.0;
    for (auto* c : possible_moves) {
        //only nodes that were visited
        if (c->visits > 0) {
            //win rate
            double rate = c->wins / c->visits;
            //update best rate if new best is found
            if (rate > bestRate) {
                bestRate = rate;
                best = c;
            }
        }
    }
    return best;
}



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