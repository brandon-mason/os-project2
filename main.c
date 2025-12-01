#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DECK_START_AMT 52

typedef struct gameData gameData_t;

typedef struct
{
    int size;
    int cards[DECK_START_AMT];
    int shuffled;
} deck_t;

typedef struct
{
    int id;
    int winner;
    pthread_t thread;
    int hand[DECK_START_AMT];
    size_t handLen;
    gameData_t *gameData;
} player_t;

struct gameData
{
    int numPlayers;
    int numCookies;
    int hasPlayed;
    int roundWon;
    int cookiesLeft;
    int roundsCompleted;

    int currentDealer;
    int currentPlayer;
    pthread_mutex_t mutexTurn;
    pthread_cond_t condTurn;

    player_t *players;

    deck_t deck;
    pthread_mutex_t mutexShuffled;
    pthread_cond_t condShuffled;

    int cookieMaster;
    pthread_mutex_t mutexMaster;
    pthread_cond_t condMaster;

    int roundCount;
    int turnCount;
    pthread_mutex_t mutexRound;
    pthread_cond_t condRound;

    pthread_mutex_t mutexDealPhase;
    pthread_cond_t condDealPhase;

    int cardsDealt;
    pthread_mutex_t mutexDeal;
    pthread_cond_t condDeal;

    int printCount;
    int printedCount;
    pthread_mutex_t mutexPrint;
    pthread_cond_t condPrint;
};

int selectTop(deck_t *deck)
{
    int topCard = deck->cards[0];
    deck->size--;
    for (int i = 0; i < deck->size; i++)
    {
        deck->cards[i] = deck->cards[i + 1];
    }
    return topCard;
}

// Easiest way I could solve the issue I was having with '10'
char *printCard(int cardNum)
{
    switch (cardNum)
    {
    case 1:
        return "A";
    case 2:
        return "2";
    case 3:
        return "3";
    case 4:
        return "4";
    case 5:
        return "5";
    case 6:
        return "6";
    case 7:
        return "7";
    case 8:
        return "8";
    case 9:
        return "9";
    case 10:
        return "10";
    case 11:
        return "J";
    case 12:
        return "Q";
    case 13:
        return "K";
    default:
        return "?";
    }
}

void showDeck(deck_t *deck)
{
    printf("DECK: ");

    for (int i = 0; i < deck->size; i++)
    {
        int card = deck->cards[i];
        printf("%s ", printCard(card));
    }
    printf("\n");
}

void showHand(player_t *player)
{
    printf("PLAYER 2: hand ");

    for (int i = 0; i < player->handLen; i++)
    {
        int cardIndex = player->hand[i];
        printf("%s ", printCard(cardIndex));
    }
    printf("\n");
}

void eatCookies(gameData_t *gameData, int playerId)
{
    int randCookies = rand() % 2;
    printf("PLAYER %d: eats %d Cookies\n", playerId, randCookies);
    gameData->cookiesLeft = gameData->cookiesLeft - randCookies;
    if (gameData->cookiesLeft <= 0)
        gameData->cookiesLeft = gameData->numCookies;
    printf("BAG: %d Cookies left\n", gameData->cookiesLeft);
}

void drawCard(gameData_t *gameData, player_t *player)
{
    showHand(player);

    deck_t *deck = &gameData->deck;
    int topCard = selectTop(deck);
    int handLen = player->handLen;

    player->hand[handLen] = topCard;
    player->handLen++;

    printf("PLAYER %d: draws %s\n", player->id, printCard(topCard));
}

void discardCard(player_t *player, deck_t *deck)
{
    int randomCardIndex = rand() % (player->handLen);
    int randomCard = player->hand[randomCardIndex];
    // int randomCard = player->hand[randomCardIndex];

    for (int i = randomCardIndex; i < player->handLen; i++)
    {
        player->hand[i] = player->hand[i + 1];
    }
    player->handLen--;

    deck->cards[deck->size] = randomCard;
    deck->size++;

    printf("PLAYER %d: discards %s at random\n", player->id, printCard(randomCard));
}

void playerWon(gameData_t *gameData, player_t *player)
{
    int winnerId = player->id;
    printf("PLAYER %d: wins round %d\n", winnerId, gameData->roundCount + 1);

    int currentLoserId = winnerId + 1;
    while (currentLoserId != winnerId)
    {
        printf("PLAYER %d: lost round %d\n", currentLoserId, gameData->roundCount + 1);
        currentLoserId = (currentLoserId + 1) % gameData->numPlayers;
    }
}

void shuffle(gameData_t *gameData)
{
    deck_t *deck = &gameData->deck;

    if (deck->size > 1)
    {
        pthread_mutex_lock(&gameData->mutexShuffled);
        while (deck->shuffled == 1)
        {
            pthread_cond_wait(&gameData->condShuffled, &gameData->mutexShuffled);
        }
        for (size_t i = 0; i < deck->size - 1; i++)
        {
            size_t j = i + rand() / (RAND_MAX / (deck->size - i) + 1);
            int t = deck->cards[j];
            deck->cards[j] = deck->cards[i];
            deck->cards[i] = t;
        }
        deck->shuffled = 1;
        pthread_mutex_unlock(&gameData->mutexShuffled);
    }
    return;
}

void choose(gameData_t *gameData)
{
    deck_t *deck = &gameData->deck;

    pthread_mutex_lock(&gameData->mutexShuffled);
    while (deck->shuffled == 0)
    {
        printf("Choose Waiting...\n");
        pthread_cond_wait(&gameData->condShuffled, &gameData->mutexShuffled);
    }
    deck->shuffled = 0;
    pthread_mutex_unlock(&gameData->mutexShuffled);

    pthread_mutex_lock(&gameData->mutexMaster);
    gameData->cookieMaster = selectTop(deck);
    pthread_mutex_unlock(&gameData->mutexMaster);

    printf("COOKIE MASTER: %s\n", printCard(gameData->cookieMaster));

    return;
}

void deal(gameData_t *gameData)
{
    deck_t *deck = &gameData->deck;

    pthread_mutex_lock(&gameData->mutexMaster);
    while (gameData->cookieMaster < 0)
    {
        printf("Deal Waiting...\n");
        pthread_cond_wait(&gameData->condMaster, &gameData->mutexMaster);
    }
    pthread_mutex_unlock(&gameData->mutexMaster);

    int playerNum = (gameData->currentDealer + 1) % gameData->numPlayers;
    while (playerNum != gameData->currentDealer)
    {
        player_t *player = &gameData->players[playerNum];
        int topCard = selectTop(deck);
        int handLen = gameData->players[playerNum].handLen;

        pthread_mutex_lock(&gameData->mutexMaster);
        gameData->players[playerNum].hand[handLen] = topCard;
        gameData->players[playerNum].handLen++;
        pthread_mutex_unlock(&gameData->mutexMaster);

        playerNum = (playerNum + 1) % gameData->numPlayers;
    }
    return;
}

int compareHand(int cookieMaster, player_t *player)
{
    for (int i = 0; i < player->handLen; i++)
    {
        if (player->hand[i] == cookieMaster)
        {
            return i;
        }
    }
    return -1;
}

void *play(void *arg)
{
    player_t *player = (player_t *)arg;
    gameData_t *gameData = player->gameData;
    int id = player->id;

    while (gameData->roundCount != gameData->numPlayers)
    {

        // ========== DEALING PHASE ==========
        pthread_mutex_lock(&gameData->mutexDealPhase);
        if (id == gameData->currentDealer && gameData->cardsDealt == 0)
        {
            printf("%d---- Round %d Start ----\n", id, gameData->roundCount + 1);
            shuffle(gameData);
            choose(gameData);
            deal(gameData);
            printf("%d DEALT\n", id);
            gameData->cardsDealt = 1;
            pthread_cond_broadcast(&gameData->condDealPhase);
        }
        else
        {
            while (gameData->cardsDealt == 0)
            {
                pthread_cond_wait(&gameData->condDealPhase, &gameData->mutexDealPhase);
            }
        }
        pthread_mutex_unlock(&gameData->mutexDealPhase);

        // ========== WAIT FOR TURN ==========
        pthread_mutex_lock(&gameData->mutexTurn);
        while (id != gameData->currentPlayer && id != gameData->currentDealer)
        {
            pthread_cond_wait(&gameData->condTurn, &gameData->mutexTurn);
        }
        pthread_mutex_unlock(&gameData->mutexTurn);

        // ========== PLAY TURN ==========
        pthread_mutex_lock(&gameData->mutexTurn);
        if (id != gameData->currentDealer && gameData->roundWon == 0)
        {
            eatCookies(gameData, id);
            drawCard(gameData, player);
            int win = compareHand(gameData->cookieMaster, player);
            if (win != -1)
            {
                showHand(player);
                printf("Cookie Master card is %d\n", gameData->cookieMaster);
                gameData->roundWon = 1;
                player->winner = 1;
            }
            else
            {
                int random = rand() % (player->handLen + 1);
                discardCard(player, &gameData->deck);
                showHand(player);
                showDeck(&gameData->deck);
            }
        }
        pthread_mutex_unlock(&gameData->mutexTurn);

        // ========== UPDATE GAME STATE ==========
        pthread_mutex_lock(&gameData->mutexRound);
        gameData->hasPlayed++;

        if (gameData->hasPlayed == gameData->numPlayers)
        {
            pthread_mutex_unlock(&gameData->mutexRound);
            pthread_mutex_lock(&gameData->mutexDealPhase);
            pthread_mutex_lock(&gameData->mutexTurn);
            pthread_mutex_lock(&gameData->mutexRound);
            gameData->currentDealer = (gameData->currentDealer + 1) % gameData->numPlayers;
            gameData->currentPlayer = (gameData->currentDealer + 1) % gameData->numPlayers;
            gameData->hasPlayed = 0;
            gameData->cardsDealt = 0;
            gameData->roundWon = 0;
            gameData->roundsCompleted++;
            gameData->cookieMaster = -1;
            pthread_mutex_unlock(&gameData->mutexRound);
            pthread_mutex_unlock(&gameData->mutexTurn);
            pthread_mutex_unlock(&gameData->mutexDealPhase);
        }
        else if (id != gameData->currentDealer)
        {
            pthread_mutex_unlock(&gameData->mutexRound);
            pthread_mutex_lock(&gameData->mutexTurn);
            if (gameData->roundWon == 0)
            {
                eatCookies(gameData, id);
            }
            gameData->currentPlayer = (gameData->currentPlayer + 1) % gameData->numPlayers;
            pthread_cond_broadcast(&gameData->condTurn);
            pthread_mutex_unlock(&gameData->mutexTurn);
        }
        else
        {
            pthread_mutex_unlock(&gameData->mutexRound);
        }

        // ========== END OF ROUND SYNCHRONIZATION ==========
        pthread_mutex_lock(&gameData->mutexRound);
        int roundCountCp = gameData->roundCount;

        gameData->turnCount++;

        if (gameData->turnCount == gameData->numPlayers)
        {
            gameData->turnCount = 0;
            gameData->roundCount++;

            pthread_cond_broadcast(&gameData->condRound);
        }
        else
        {
            while (gameData->roundCount == roundCountCp)
            {
                pthread_cond_wait(&gameData->condRound, &gameData->mutexRound);
            }
        }
        pthread_mutex_unlock(&gameData->mutexRound);

        pthread_mutex_lock(&gameData->mutexPrint);
        pthread_mutex_lock(&gameData->mutexTurn);
        if (player->winner == 1)
        {
            printf("PLAYER %d: wins round %d\n", id + 1, gameData->roundCount);
        }
        else
        {
            printf("PLAYER %d: lost round %d\n", id + 1, gameData->roundCount);
        }
        player->winner = 0;
        pthread_cond_broadcast(&gameData->condTurn);
        pthread_mutex_unlock(&gameData->mutexTurn);

        int printCountCp = gameData->printCount;

        gameData->printedCount++;

        if (gameData->printedCount == gameData->numPlayers)
        {
            gameData->printedCount = 0;
            gameData->printCount++;

            pthread_cond_broadcast(&gameData->condPrint);
        }
        else
        {
            while (gameData->printCount == printCountCp)
            {
                pthread_cond_wait(&gameData->condPrint, &gameData->mutexPrint);
            }
        }
        pthread_mutex_unlock(&gameData->mutexPrint);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int seed = atoi(argv[1]);
    int numPlayers = atoi(argv[2]);
    int numCookies = atoi(argv[3]);
    srand(seed);

    deck_t deck;
    deck.cards[DECK_START_AMT];
    deck.size = DECK_START_AMT;
    deck.shuffled = 0;

    for (int i = 1; i < DECK_START_AMT; i++)
    {
        deck.cards[i] = (i / 4) + 1;
    }

    gameData_t gameData;
    gameData.numPlayers = numPlayers;
    gameData.numCookies = numCookies;
    gameData.cookiesLeft = numCookies;
    gameData.roundsCompleted = 0;
    gameData.cookieMaster = -1;
    gameData.deck = deck;
    gameData.currentDealer = 0;
    gameData.currentPlayer = 1;
    gameData.cardsDealt = 0;
    gameData.hasPlayed = 0;
    gameData.roundWon = 0;
    gameData.roundCount = 0;
    gameData.turnCount = 0;
    gameData.printCount = 0;
    gameData.printedCount = 0;

    pthread_mutex_t mutexShuffled, mutexMaster, mutexTurn, mutexRound, mutexDealPhase, mutexDeal, mutexPrint;
    pthread_cond_t condShuffled, condMaster, condTurn, condRound, condDealPhase, condDeal, condPrint;

    pthread_mutex_init(&mutexShuffled, NULL);
    pthread_cond_init(&condShuffled, NULL);
    gameData.mutexShuffled = mutexShuffled;
    gameData.condShuffled = condShuffled;

    pthread_mutex_init(&mutexMaster, NULL);
    pthread_cond_init(&condMaster, NULL);
    gameData.mutexMaster = mutexMaster;
    gameData.condMaster = condMaster;

    pthread_mutex_init(&mutexTurn, NULL);
    pthread_cond_init(&condTurn, NULL);
    gameData.mutexTurn = mutexTurn;
    gameData.condTurn = condTurn;

    pthread_mutex_init(&mutexRound, NULL);
    pthread_cond_init(&condRound, NULL);
    gameData.mutexRound = mutexRound;
    gameData.condRound = condRound;

    pthread_mutex_init(&mutexPrint, NULL);
    pthread_cond_init(&condPrint, NULL);
    gameData.mutexPrint = mutexPrint;
    gameData.condPrint = condPrint;

    pthread_mutex_init(&mutexDealPhase, NULL);
    pthread_cond_init(&condDealPhase, NULL);
    gameData.condDealPhase = condDealPhase;
    gameData.mutexDealPhase = mutexDealPhase;

    pthread_mutex_init(&gameData.mutexDeal, NULL);
    pthread_cond_init(&gameData.condDeal, NULL);
    gameData.condDeal = condDeal;
    gameData.mutexDeal = mutexDeal;

    gameData.players = malloc(sizeof(player_t) * gameData.numPlayers);
    player_t *players = gameData.players;

    for (int i = 0; i < numPlayers; i++)
    {
        players[i].id = i;
        players[i].handLen = 0;
        players[i].gameData = &gameData;

        pthread_create(&players[i % numPlayers].thread, NULL, play, &players[i % numPlayers]);
    }
    for (int i = 0; i < numPlayers; i++)
    {
        pthread_join(players[i % numPlayers].thread, NULL);
    }

    return 0;
}