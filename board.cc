#include "board.hh"

#include <algorithm>
#include <assert.h>
#include <cmath>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>

#include "piece.hh"

Board::Board(size_t size)
    : size(size)
    , board(size, std::vector<std::optional<Piece>>(size))
    , numPieces(size * size)
    , numEdges(4 * size * size)
    , lastRandomSwap(std::nullopt)
    , lastLoss(std::nullopt)
    , backupLoss(std::nullopt)
    , lossIsComputed(false)
{}

void Board::setPiece(size_t x, size_t y, Piece piece)
{
    if (x >= size || y >= size)
    {
        throw std::invalid_argument("Invalid coordinates");
    }
    board[x][y] = piece;
}

std::optional<Piece> Board::getPiece(long long int x, long long int y) const
{
    if (x < 0 || y < 0)
    {
        return std::nullopt;
    }

    if (static_cast<size_t>(x) >= size || static_cast<size_t>(y) >= size)
    {
        return std::nullopt;
    }

    return board[x][y];
}

std::optional<Piece> Board::getPiece(const Coords &coords) const
{
    return getPiece(coords.x, coords.y);
}

std::string Board::getBoardSeperator(size_t size)
{
    std::string linePattern = "+" + std::string(5, '-');
    std::string buffer = "";
    for (size_t i = 0; i < size; i++)
    {
        buffer += linePattern;
    }
    return buffer + "+\n";
}

Board Board::fromString(std::string input)
{
    std::vector<Piece> pieces;
    std::istringstream stream(input);
    std::string line;
    while (std::getline(stream, line))
    {
        if (line.empty())
        {
            continue;
        }
        pieces.push_back(Piece::fromString(line));
    }

    size_t numPieces = pieces.size();
    size_t size = sqrt(numPieces);

    if (size == 0 || size * size != numPieces)
    {
        throw std::invalid_argument("Invalid board size");
    }

    Board board = Board(size);
    for (size_t i = 0; i < size; i++)
    {
        for (size_t j = 0; j < size; j++)
        {
            board.setPiece(i, j, pieces[i * size + j]);
        }
    }
    return board;
}

std::string Board::toString() const
{
    std::string buffer = "";
    for (size_t i = 0; i < size; i++)
    {
        for (size_t j = 0; j < size; j++)
        {
            if (board[i][j].has_value())
            {
                buffer += board[i][j].value().toString();
            }
            buffer += "\n";
        }
    }
    return buffer;
}

size_t Board::getSize() const
{
    return size;
}

void Board::print() const
{
    for (size_t i = 0; i < size; i++)
    {
        for (size_t j = 0; j < size; j++)
        {
            if (board[i][j].has_value())
            {
                std::cout << board[i][j].value() << std::endl;
            }
            else
            {
                std::cout << "-x-" << std::endl;
            }
        }
    }
}

std::string Board::getBoardOnePieceLine(std::vector<size_t> numberList)
{
    std::string buffer = "";
    for (size_t i = 0; i < numberList.size(); i++)
    {
        buffer += "|  " + std::to_string(numberList[i]) + "  ";
    }
    return buffer + "|\n";
}

std::string Board::getBoardTwoPieceLine(std::vector<size_t> firstNumberList,
                                        std::vector<bool> anchorList,
                                        std::vector<size_t> secondNumberList)
{
    std::string buffer = "";
    for (size_t i = 0; i < firstNumberList.size(); i++)
    {
        buffer += "| " + std::to_string(firstNumberList[i]);
        buffer += anchorList[i] ? "@" : " ";
        buffer += std::to_string(secondNumberList[i]) + " ";
    }
    return buffer + "|\n";
}

std::ostream &operator<<(std::ostream &os, const Board &board)
{
    os << Board::getBoardSeperator(board.getSize());
    for (size_t i = 0; i < board.getSize(); i++)
    {
        std::vector<size_t> northNumberList;
        std::vector<size_t> westNumberList;
        std::vector<size_t> eastNumberList;
        std::vector<size_t> southNumberList;
        std::vector<bool> anchorList;
        for (size_t j = 0; j < board.getSize(); j++)
        {
            std::optional<Piece> piece = board.getPiece(i, j);
            if (!piece.has_value())
            {
                piece = Piece(0, 0, 0, 0, true);
            }
            northNumberList.push_back(piece.value().getNorth());
            westNumberList.push_back(piece.value().getWest());
            eastNumberList.push_back(piece.value().getEast());
            southNumberList.push_back(piece.value().getSouth());
            anchorList.push_back(piece.value().getIsAnchored());
        }
        os << Board::getBoardOnePieceLine(northNumberList);
        os << Board::getBoardTwoPieceLine(westNumberList, anchorList,
                                          eastNumberList);
        os << Board::getBoardOnePieceLine(southNumberList);
        os << Board::getBoardSeperator(board.getSize());
    }
    return os;
}

std::vector<Coords> Board::getMovablePieceCoordsList() const
{
    std::vector<Coords> coordsList;
    for (size_t i = 0; i < this->size; i++)
    {
        for (size_t j = 0; j < this->size; j++)
        {
            std::optional<Piece> piece = this->getPiece(i, j);
            if (piece.has_value() && !piece.value().getIsAnchored())
            {
                coordsList.push_back(Coords(i, j));
            }
        }
    }
    return coordsList;
}

std::pair<Coords, Coords> Board::get2RandomMovablePieceCoords() const
{
    // Get first random piece
    auto movablePieceCount = this->movablePieceCoordsList.size();
    auto firstRandomIndex =
        static_cast<size_t>(randomFloat(0, movablePieceCount));
    auto secondRandomIndex =
        static_cast<size_t>(randomFloat(0, movablePieceCount));
    if (firstRandomIndex == secondRandomIndex)
    {
        secondRandomIndex = (secondRandomIndex + 1) % movablePieceCount;
    }
    return std::make_pair(this->movablePieceCoordsList[firstRandomIndex],
                          this->movablePieceCoordsList[secondRandomIndex]);
}

void Board::randomSwap()
{
    std::pair<Coords, Coords> coordsPair = this->get2RandomMovablePieceCoords();
    this->lastRandomSwap = coordsPair;
    this->backupLoss = this->loss();
    this->swap(coordsPair.first, coordsPair.second);
    this->lossIsComputed = false;
}

void Board::reverseLastRandomSwap()
{
    if (this->lastRandomSwap.has_value())
    {
        this->swap(this->lastRandomSwap.value().first,
                   this->lastRandomSwap.value().second);
        this->lastRandomSwap = std::nullopt;
        this->lossIsComputed = true;
        this->lastLoss = this->backupLoss;
        this->backupLoss = std::nullopt;
    }
}

void Board::swap(const Coords &coords1, const Coords &coords2)
{
    size_t x1 = coords1.x;
    size_t y1 = coords1.y;
    size_t x2 = coords2.x;
    size_t y2 = coords2.y;
    std::optional<Piece> temp = this->board[x1][y1];
    this->board[x1][y1] = this->board[x2][y2];
    this->board[x2][y2] = temp;
}

float Board::loss()
{
    if (this->lossIsComputed)
    {
        return this->lastLoss.value();
    }

    float loss = 0;
    this->lossIsComputed = true;
    if (this->lastLoss.has_value())
    {
        loss = this->localLoss();
        // assert(loss == this->globalLoss());
    }
    else
    {
        loss = this->globalLoss();
    }

    this->lastLoss = loss;
    return loss;
}

float Board::globalLoss() const
{
    float loss = 0;

    for (size_t i = 0; i < this->size; i++)
    {
        for (size_t j = 0; j < this->size; j++)
        {
            std::optional<Piece> piece = this->getPiece(i, j);
            if (!piece.has_value())
            {
                continue;
            }
            loss += this->pieceLoss(Coords(i, j), piece.value());
        }
    }
    return loss / this->numEdges;
}

float Board::localLoss()
{
    auto coords1 = this->lastRandomSwap.value().first;
    auto coords2 = this->lastRandomSwap.value().second;
    float loss = this->lastLoss.value() * this->numEdges;

    // Remove the loss of the two swapped pieces BEFORE the swap
    this->swap(coords1, coords2);
    loss -= this->pieceCrossLoss(coords1, coords2);
    loss -= this->pieceCrossLoss(coords2, coords1);

    // Add the loss of the two swapped pieces AFTER the swap
    this->swap(coords1, coords2);
    loss += this->pieceCrossLoss(coords1, coords2);
    loss += this->pieceCrossLoss(coords2, coords1);

    loss = (loss < 1) ? 0 : loss; // Because of floating point errors
    return loss / this->numEdges;
}

float Board::pieceCrossLoss(const Coords &firstCoords,
                            const Coords &secondCoords) const
{
    float loss = 0;
    auto piece = this->getPiece(firstCoords.x, firstCoords.y).value();
    Coords topCoords = Coords(firstCoords.x - 1, firstCoords.y);
    std::optional<Piece> topPiece = this->getPiece(topCoords);
    Coords bottomCoords = Coords(firstCoords.x + 1, firstCoords.y);
    std::optional<Piece> bottomPiece = this->getPiece(bottomCoords);
    Coords leftCoords = Coords(firstCoords.x, firstCoords.y - 1);
    std::optional<Piece> leftPiece = this->getPiece(leftCoords);
    Coords rightCoords = Coords(firstCoords.x, firstCoords.y + 1);
    std::optional<Piece> rightPiece = this->getPiece(rightCoords);

    // Check if two edges are connected
    // TOP
    if (topPiece.has_value() && piece.getNorth() != topPiece.value().getSouth())
    {
        loss += topCoords == secondCoords ? 1 : 2;
    }
    // BOTTOM
    if (bottomPiece.has_value()
        && piece.getSouth() != bottomPiece.value().getNorth())
    {
        loss += bottomCoords == secondCoords ? 1 : 2;
    }
    // LEFT
    if (leftPiece.has_value() && piece.getWest() != leftPiece.value().getEast())
    {
        loss += leftCoords == secondCoords ? 1 : 2;
    }
    // RIGHT
    if (rightPiece.has_value()
        && piece.getEast() != rightPiece.value().getWest())
    {
        loss += rightCoords == secondCoords ? 1 : 2;
    }

    return loss;
}

float Board::pieceLoss(const Coords &coords, const Piece &piece) const
{
    float loss = 0;
    std::optional<Piece> topPiece = this->getPiece(coords.x - 1, coords.y);
    std::optional<Piece> bottomPiece = this->getPiece(coords.x + 1, coords.y);
    std::optional<Piece> leftPiece = this->getPiece(coords.x, coords.y - 1);
    std::optional<Piece> rightPiece = this->getPiece(coords.x, coords.y + 1);

    // Check if two edges are connected
    // TOP
    if (topPiece.has_value() && piece.getNorth() != topPiece.value().getSouth())
    {
        loss += 1;
    }
    // BOTTOM
    if (bottomPiece.has_value()
        && piece.getSouth() != bottomPiece.value().getNorth())
    {
        loss += 1;
    }
    // LEFT
    if (leftPiece.has_value() && piece.getWest() != leftPiece.value().getEast())
    {
        loss += 1;
    }
    // RIGHT
    if (rightPiece.has_value()
        && piece.getEast() != rightPiece.value().getWest())
    {
        loss += 1;
    }

    return loss;
}

Board Board::copy() const
{
    return Board(*this);
}

float normalizedRandom()
{
    return std::rand() / static_cast<float>(RAND_MAX);
}

float randomFloat(float min, float max)
{
    return min + (max - min) * normalizedRandom();
}

bool Board::solve(const size_t epochMax, const float initialTemperature,
                  const float coldFactor, const float heatFactor,
                  const size_t stuckCounterMax, const bool verbose = false)
{
    auto p = [&](float delta, float temperature) -> float {
        return std::exp(-delta / temperature);
    };

    this->movablePieceCoordsList = this->getMovablePieceCoordsList();
    Board optimalBoard = this->copy();
    float loss = this->loss();
    float optimalLoss = optimalBoard.loss();
    size_t epoch = 0;
    float temperature = initialTemperature;
    bool success = false;
    size_t epochPerPercent = epochMax / 100;
    const bool useReheating = (this->size >= 5);
    size_t stuckCounter = 0;

    for (epoch = 0; epoch < epochMax; epoch++)
    {
        this->randomSwap();
        float epochLoss = this->loss();
        float lossDelta = std::abs(epochLoss - loss);

        if (epochLoss < loss || normalizedRandom() < p(lossDelta, temperature))
        {
            loss = epochLoss;
            stuckCounter = 0;
            // Second optimisation
            // Because some time we fall in local minimum and the temperature
            // continue to decrease The resolver is stuck in a local minimum To
            // avoid this, we can freeze the temperature each time we fail to
            // find a better solution So when we are stuck in a local minimum,
            // the temperature is high enough to escape at some point
            // temperature *= coldFactor;
            if (!useReheating)
            {
                temperature *= coldFactor;
            }
        }
        else
        {
            // First optimisation
            // Instead of copying the board and replace by the previous one
            // we can just reverse the last swap
            // With this optimisation, we can go from 4s to 1s
            this->reverseLastRandomSwap();
            stuckCounter++;
        }

        // Third optimisation
        // When the board is big, the temperature decrease too fast
        // And we are stuck in a local minimum
        // So we can increase the temperature when we are stuck
        if (useReheating)
        {
            if (stuckCounter >= stuckCounterMax)
            {
                temperature /= heatFactor;
                stuckCounter = 0;
            }
            else
            {
                temperature *= coldFactor;
            }
        }

        if (loss < optimalLoss)
        {
            optimalLoss = loss;
            optimalBoard = this->copy();
        }

        if (epoch % epochPerPercent == 0 && verbose)
        {
            std::cout << "[" << epoch << "] Optimal loss : " << optimalLoss
                      << " | temp : " << temperature << std::endl;
        }

        if (optimalLoss == 0)
        {
            if (verbose)
            {
                std::cout << "[" << epoch << "] Optimal loss : " << optimalLoss
                          << std::endl;
            }
            success = true;
            break;
        }
    }

    this->board = optimalBoard.board;
    if (!success)
    {
        std::cerr << "Unable to solve the board !" << std::endl;
        std::cout << "  * Temperature : " << temperature << std::endl;
        std::cout << "  * Loss : " << optimalLoss << std::endl;

        this->board = optimalBoard.board;
    }
    return success;
}