#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <ctime>
#include <limits>
#include <map>

using namespace std;

// Color codes
const string RED = "\033[31m";
const string GREEN = "\033[32m";
const string YELLOW = "\033[33m";
const string BLUE = "\033[34m";
const string PINK = "\033[35m";
const string CYAN = "\033[36m";
const string WHITE = "\033[37m";
const string RESET = "\033[0m";
const string BOLD = "\033[1m";

// Card types
enum class CardType { PROPERTY, MONEY, ACTION, RENT, WILD };

// Property colors (simplified names)
enum class PropertyColor { BROWN, BLUE, PINK, ORANGE, RED, YELLOW, GREEN, DARKBLUE, UTILITY, RAILROAD, NONE };

// Property set requirements
const map<PropertyColor, pair<int, string>> PROPERTY_SETS = {
    {PropertyColor::BROWN, {2, "Brown (2)"}},
    {PropertyColor::BLUE, {3, "Blue (3)"}},
    {PropertyColor::PINK, {3, "Pink (3)"}},
    {PropertyColor::ORANGE, {3, "Orange (3)"}},
    {PropertyColor::RED, {3, "Red (3)"}},
    {PropertyColor::YELLOW, {3, "Yellow (3)"}},
    {PropertyColor::GREEN, {3, "Green (3)"}},
    {PropertyColor::DARKBLUE, {2, "Dark Blue (2)"}},
    {PropertyColor::UTILITY, {2, "Utility (2)"}},
    {PropertyColor::RAILROAD, {4, "Railroad (4)"}}
};

// Card structure
struct Card {
    string name;
    CardType type;
    int value;
    PropertyColor color;
    bool isWild;
    vector<PropertyColor> wildColors;
    
    Card(string n, CardType t, int v, PropertyColor c = PropertyColor::NONE, 
         bool wild = false, vector<PropertyColor> w = {})
        : name(n), type(t), value(v), color(c), isWild(wild), wildColors(w) {}
};

class Player {
private:
    string name;
    vector<Card> hand;
    map<PropertyColor, vector<Card>> properties;
    vector<Card> wildCards;
    int money;
    bool hasJustSayNo;
    
public:
    Player(string n) : name(n), money(0), hasJustSayNo(false) {}
    
    string getName() const { return name; }
    vector<Card>& getHand() { return hand; }
    int getMoney() const { return money; }
    bool hasJustSayNoCard() const { return hasJustSayNo; }
    
    void addToHand(Card card) {
        hand.push_back(card);
        if (card.name == "Just Say No") hasJustSayNo = true;
    }
    
    bool isCompleteSet(PropertyColor color) const {
        auto it = PROPERTY_SETS.find(color);
        if (it == PROPERTY_SETS.end()) return false;
        
        int count = properties.count(color) ? properties.at(color).size() : 0;
        for (const auto& wild : wildCards) {
            if (wild.color == color) count++;
        }
        return count >= it->second.first;
    }
    
    vector<PropertyColor> getCompleteSets() const {
        vector<PropertyColor> complete;
        for (const auto& [color, _] : PROPERTY_SETS) {
            if (isCompleteSet(color)) complete.push_back(color);
        }
        return complete;
    }
    
    vector<PropertyColor> getStealableProperties(bool completeOnly) const {
        vector<PropertyColor> stealable;
        for (const auto& [color, _] : properties) {
            if ((completeOnly && isCompleteSet(color)) || 
                (!completeOnly && !properties.at(color).empty())) {
                stealable.push_back(color);
            }
        }
        return stealable;
    }
    
    string getColorCode(PropertyColor color) const {
        switch(color) {
            case PropertyColor::BROWN: return "\033[48;5;94m";
            case PropertyColor::BLUE: return "\033[48;5;117m";
            case PropertyColor::PINK: return "\033[48;5;218m";
            case PropertyColor::ORANGE: return "\033[48;5;214m";
            case PropertyColor::RED: return "\033[48;5;196m";
            case PropertyColor::YELLOW: return "\033[48;5;226m";
            case PropertyColor::GREEN: return "\033[48;5;46m";
            case PropertyColor::DARKBLUE: return "\033[48;5;21m";
            case PropertyColor::UTILITY: return "\033[48;5;255m";
            case PropertyColor::RAILROAD: return "\033[48;5;240m";
            default: return "\033[48;5;15m";
        }
    }
    
    string getCardColor(const Card& card) const {
        switch(card.type) {
            case CardType::MONEY: return YELLOW;
            case CardType::ACTION: return PINK;
            case CardType::RENT: return RED;
            case CardType::PROPERTY: return getColorCode(card.color);
            default: return WHITE;
        }
    }
    
    bool playCard(int index, Player& opponent) {
        if (index < 0 || index >= hand.size()) return false;
        
        Card card = hand[index];
        bool played = true;
        
        switch(card.type) {
            case CardType::MONEY:
                money += card.value;
                break;
                
            case CardType::PROPERTY:
                if (card.isWild) {
                    cout << "Choose color for wild card:\n";
                    int i = 0;
                    for (const auto& [color, req] : PROPERTY_SETS) {
                        cout << i++ << ": " << getColorCode(color) << req.second << RESET << "\n";
                    }
                    int choice;
                    while (!(cin >> choice)) {
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        cout << "Invalid input. Try again: ";
                    }
                    if (choice >= 0 && choice < PROPERTY_SETS.size()) {
                        auto it = PROPERTY_SETS.begin();
                        advance(it, choice);
                        card.color = it->first;
                        wildCards.push_back(card);
                    }
                } else {
                    properties[card.color].push_back(card);
                }
                break;
                
            case CardType::RENT: {
                if (opponent.hasJustSayNoCard()) {
                    cout << PINK << opponent.name << " plays Just Say No! Rent blocked!\n" << RESET;
                    played = false;
                    break;
                }
                int rent = 0;
                for (const auto& [color, _] : properties) {
                    if (isCompleteSet(color)) rent += PROPERTY_SETS.at(color).first * 2;
                }
                rent = min(rent, opponent.money);
                money += rent;
                opponent.money -= rent;
                cout << YELLOW << "Collected $" << rent << " rent from " << opponent.name << "\n" << RESET;
                break;
            }
                
            case CardType::ACTION:
                if (card.name == "Deal Breaker") {
                    auto sets = opponent.getCompleteSets();
                    if (sets.empty()) {
                        cout << RED << "No complete sets to steal!\n" << RESET;
                        played = false;
                        break;
                    }
                    cout << "Choose set to steal:\n";
                    for (size_t i = 0; i < sets.size(); i++) {
                        cout << i << ": " << getColorCode(sets[i]) << PROPERTY_SETS.at(sets[i]).second << RESET << "\n";
                    }
                    int choice;
                    cin >> choice;
                    if (choice >= 0 && choice < sets.size()) {
                        PropertyColor color = sets[choice];
                        properties[color] = opponent.properties[color];
                        opponent.properties.erase(color);
                        cout << GREEN << "Stole " << PROPERTY_SETS.at(color).second << " set!\n" << RESET;
                    } else {
                        played = false;
                    }
                }
                else if (card.name == "Sly Deal") {
                    auto props = opponent.getStealableProperties(false);
                    if (props.empty()) {
                        cout << RED << "No properties to steal!\n" << RESET;
                        played = false;
                        break;
                    }
                    cout << "Choose property to steal:\n";
                    for (size_t i = 0; i < props.size(); i++) {
                        cout << i << ": " << getColorCode(props[i]) << PROPERTY_SETS.at(props[i]).second << RESET << "\n";
                    }
                    int choice;
                    cin >> choice;
                    if (choice >= 0 && choice < props.size()) {
                        PropertyColor color = props[choice];
                        properties[color].push_back(opponent.properties[color].back());
                        opponent.properties[color].pop_back();
                        cout << GREEN << "Stole a " << PROPERTY_SETS.at(color).second << " property!\n" << RESET;
                    } else {
                        played = false;
                    }
                }
                else if (card.name == "Forced Deal") {
                    auto myProps = getStealableProperties(false);
                    auto theirProps = opponent.getStealableProperties(false);
                    
                    if (myProps.empty() || theirProps.empty()) {
                        cout << RED << "Not enough properties to trade!\n" << RESET;
                        played = false;
                        break;
                    }
                    
                    cout << "Choose your property to give:\n";
                    for (size_t i = 0; i < myProps.size(); i++) {
                        cout << i << ": " << getColorCode(myProps[i]) << PROPERTY_SETS.at(myProps[i]).second << RESET << "\n";
                    }
                    int myChoice;
                    cin >> myChoice;
                    
                    cout << "Choose their property to take:\n";
                    for (size_t i = 0; i < theirProps.size(); i++) {
                        cout << i << ": " << getColorCode(theirProps[i]) << PROPERTY_SETS.at(theirProps[i]).second << RESET << "\n";
                    }
                    int theirChoice;
                    cin >> theirChoice;
                    
                    if (myChoice >= 0 && myChoice < myProps.size() &&
                        theirChoice >= 0 && theirChoice < theirProps.size()) {
                        PropertyColor give = myProps[myChoice];
                        PropertyColor take = theirProps[theirChoice];
                        
                        opponent.properties[give].push_back(properties[give].back());
                        properties[give].pop_back();
                        properties[take].push_back(opponent.properties[take].back());
                        opponent.properties[take].pop_back();
                        
                        cout << GREEN << "Traded properties!\n" << RESET;
                    } else {
                        played = false;
                    }
                }
                break;
        }
        
        if (played) {
            hand.erase(hand.begin() + index);
            if (card.name == "Just Say No") hasJustSayNo = false;
        }
        
        return played;
    }
    
    void displayStatus() const {
        cout << BOLD << "\nPlayer: " << name << RESET << "\n";
        cout << YELLOW << "Money: $" << money << RESET << "\n";
        
        cout << "Properties:\n";
        for (const auto& [color, cards] : properties) {
            cout << " - " << getColorCode(color) << PROPERTY_SETS.at(color).second << RESET << ": ";
            for (const auto& card : cards) cout << card.name << " ";
            if (isCompleteSet(color)) {
                cout << GREEN << "(Complete)";
            } else {
                cout << RED << "(Incomplete)";
            }
            cout << RESET << "\n";
        }
        
        cout << "Hand (" << hand.size() << " cards):\n";
        for (size_t i = 0; i < hand.size(); i++) {
            cout << i << ": " << getCardColor(hand[i]) << hand[i].name << RESET << "\n";
        }
    }
};

class MonopolyDealGame {
private:
    vector<Player> players;
    vector<Card> drawPile;
    vector<Card> discardPile;
    size_t currentPlayer;
    
    void initializeDeck() {
        // Property cards (28)
        addPropertyCards("Mediterranean", 1, PropertyColor::BROWN, 2);
        addPropertyCards("Baltic", 1, PropertyColor::BROWN, 2);
        addPropertyCards("Oriental", 1, PropertyColor::BLUE, 3);
        addPropertyCards("Vermont", 1, PropertyColor::BLUE, 3);
        addPropertyCards("Connecticut", 1, PropertyColor::BLUE, 3);
        addPropertyCards("St. Charles", 2, PropertyColor::PINK, 3);
        addPropertyCards("States", 2, PropertyColor::PINK, 3);
        addPropertyCards("Virginia", 2, PropertyColor::PINK, 3);
        addPropertyCards("St. James", 3, PropertyColor::ORANGE, 3);
        addPropertyCards("Tennessee", 3, PropertyColor::ORANGE, 3);
        addPropertyCards("New York", 3, PropertyColor::ORANGE, 3);
        addPropertyCards("Kentucky", 3, PropertyColor::RED, 3);
        addPropertyCards("Indiana", 3, PropertyColor::RED, 3);
        addPropertyCards("Illinois", 3, PropertyColor::RED, 3);
        addPropertyCards("Atlantic", 4, PropertyColor::YELLOW, 3);
        addPropertyCards("Ventnor", 4, PropertyColor::YELLOW, 3);
        addPropertyCards("Marvin", 4, PropertyColor::YELLOW, 3);
        addPropertyCards("Pacific", 4, PropertyColor::GREEN, 3);
        addPropertyCards("N. Carolina", 4, PropertyColor::GREEN, 3);
        addPropertyCards("Pennsylvania", 4, PropertyColor::GREEN, 3);
        addPropertyCards("Park Place", 5, PropertyColor::DARKBLUE, 2);
        addPropertyCards("Boardwalk", 5, PropertyColor::DARKBLUE, 2);
        addPropertyCards("Electric Co.", 2, PropertyColor::UTILITY, 2);
        addPropertyCards("Water Works", 2, PropertyColor::UTILITY, 2);
        addPropertyCards("Reading RR", 1, PropertyColor::RAILROAD, 4);
        addPropertyCards("Pennsylvania RR", 1, PropertyColor::RAILROAD, 4);
        addPropertyCards("B. & O. RR", 1, PropertyColor::RAILROAD, 4);
        addPropertyCards("Short Line", 1, PropertyColor::RAILROAD, 4);
        
        // Wild cards (6)
        drawPile.push_back(Card("Wild (Blue/Green)", CardType::PROPERTY, 0, PropertyColor::NONE, true, 
                              {PropertyColor::DARKBLUE, PropertyColor::GREEN}));
        drawPile.push_back(Card("Wild (Blue/Green)", CardType::PROPERTY, 0, PropertyColor::NONE, true, 
                              {PropertyColor::DARKBLUE, PropertyColor::GREEN}));
        drawPile.push_back(Card("Wild (Red/Yellow)", CardType::PROPERTY, 0, PropertyColor::NONE, true, 
                              {PropertyColor::RED, PropertyColor::YELLOW}));
        drawPile.push_back(Card("Wild (Red/Yellow)", CardType::PROPERTY, 0, PropertyColor::NONE, true, 
                              {PropertyColor::RED, PropertyColor::YELLOW}));
        drawPile.push_back(Card("Wild (Brown/Blue)", CardType::PROPERTY, 0, PropertyColor::NONE, true, 
                              {PropertyColor::BROWN, PropertyColor::BLUE}));
        drawPile.push_back(Card("Wild (Brown/Blue)", CardType::PROPERTY, 0, PropertyColor::NONE, true, 
                              {PropertyColor::BROWN, PropertyColor::BLUE}));
        
        // Money cards (37)
        addMoneyCards(1, 10);
        addMoneyCards(2, 10);
        addMoneyCards(3, 10);
        addMoneyCards(4, 10);
        addMoneyCards(5, 5);
        addMoneyCards(10, 2);
        
        // Action cards (37)
        for (int i = 0; i < 5; i++) drawPile.push_back(Card("Deal Breaker", CardType::ACTION, 0));
        for (int i = 0; i < 5; i++) drawPile.push_back(Card("Sly Deal", CardType::ACTION, 0));
        for (int i = 0; i < 5; i++) drawPile.push_back(Card("Forced Deal", CardType::ACTION, 0));
        for (int i = 0; i < 5; i++) drawPile.push_back(Card("Just Say No", CardType::ACTION, 0));
        for (int i = 0; i < 10; i++) drawPile.push_back(Card("Rent (1 Color)", CardType::RENT, 3));
        for (int i = 0; i < 7; i++) drawPile.push_back(Card("Rent (All Colors)", CardType::RENT, 1));
    }
    
    void addPropertyCards(string name, int value, PropertyColor color, int count) {
        for (int i = 0; i < count; i++) {
            drawPile.push_back(Card(name, CardType::PROPERTY, value, color));
        }
    }
    
    void addMoneyCards(int value, int count) {
        for (int i = 0; i < count; i++) {
            drawPile.push_back(Card("$" + to_string(value), CardType::MONEY, value));
        }
    }
    
    void reshuffle() {
        if (drawPile.empty() && !discardPile.empty()) {
            cout << CYAN << "Reshuffling discard pile into draw pile!\n" << RESET;
            drawPile = discardPile;
            discardPile.clear();
            shuffle(drawPile.begin(), drawPile.end(), default_random_engine(time(nullptr)));
        }
    }
    
public:
    MonopolyDealGame(vector<string> names) : currentPlayer(0) {
        for (auto name : names) players.emplace_back(name);
        initializeDeck();
        shuffle(drawPile.begin(), drawPile.end(), default_random_engine(time(nullptr)));
        dealInitialCards();
    }
    
    void dealInitialCards() {
        for (int i = 0; i < 5; i++) {
            for (auto& player : players) {
                if (!drawPile.empty()) {
                    player.addToHand(drawPile.back());
                    drawPile.pop_back();
                }
            }
        }
    }
    
    void playGame() {
        cout << BOLD << CYAN << "=== MONOPOLY DEAL ===\n" << RESET;
        cout << "First to 3 complete property sets wins!\n";
        
        while (true) {
            Player& current = players[currentPlayer];
            
            cout << BOLD << "\n=== " << current.getName() << "'s turn ===\n" << RESET;
            
            // Draw 2 cards
            for (int i = 0; i < 2; i++) {
                reshuffle();
                if (drawPile.empty()) {
                    cout << RED << "No cards left to draw!\n" << RESET;
                    break;
                }
                current.addToHand(drawPile.back());
                cout << GREEN << "Drew: " << drawPile.back().name << "\n" << RESET;
                drawPile.pop_back();
            }
            
            // Play up to 3 cards
            int played = 0;
            while (played < 3 && !current.getHand().empty()) {
                current.displayStatus();
                cout << YELLOW << "Play card (0-" << current.getHand().size()-1 << ") or -1 to end: " << RESET;
                int choice;
                while (!(cin >> choice)) {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "Invalid input. Try again: ";
                }
                
                if (choice == -1) break;
                
                if (current.playCard(choice, players[(currentPlayer + 1) % players.size()])) {
                    played++;
                } else {
                    cout << RED << "Invalid play! Try again.\n" << RESET;
                }
            }
            
            // Check win condition
            if (current.getCompleteSets().size() >= 3) {
                cout << BOLD << GREEN << "\n" << current.getName() << " wins with 3 complete property sets!\n" << RESET;
                break;
            }
            
            // Discard down to 7 cards
            while (current.getHand().size() > 7) {
                current.displayStatus();
                cout << RED << "Discard down to 7. Choose card (0-" << current.getHand().size()-1 << "): " << RESET;
                int choice;
                cin >> choice;
                
                if (choice >= 0 && choice < current.getHand().size()) {
                    discardPile.push_back(current.getHand()[choice]);
                    current.getHand().erase(current.getHand().begin() + choice);
                }
            }
            
            currentPlayer = (currentPlayer + 1) % players.size();
        }
    }
};

int main() {
    cout << BOLD << CYAN << "=== MONOPOLY DEAL ===\n" << RESET;
    cout << "Enter number of players (2-4): ";
    int numPlayers;
    cin >> numPlayers;
    
    if (numPlayers < 2 || numPlayers > 4) {
        cout << RED << "Invalid number. Defaulting to 2.\n" << RESET;
        numPlayers = 2;
    }
    
    vector<string> names;
    for (int i = 0; i < numPlayers; i++) {
        cout << "Player " << i+1 << " name: ";
        string name;
        cin >> name;
        names.push_back(name);
    }
    
    MonopolyDealGame game(names);
    game.playGame();
    
    return 0;
}
