#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define DECK_START_AMT 52

typedef struct gameData gameData_t;

typedef struct {
    int size;
    int cards[DECK_START_AMT];
    int shuffled;
} deck_t;

typedef struct {
    int id;
    pthread_t thread;
    int hand[DECK_START_AMT];
    size_t handLen;
    int isDealer;
    gameData_t* gameData;
} player_t;

struct gameData {
    int numPlayers;
    int numCookies;

    int currentDealer;
    int currentPlayer;
    pthread_mutex_t mutexTurn;
    pthread_cond_t condTurn;

    player_t* players;

    deck_t deck;
    pthread_mutex_t mutexShuffled;
    pthread_cond_t condShuffled;

    int cookieMaster;
    pthread_mutex_t mutexMaster;
    pthread_cond_t condMaster;

    int turn;
    pthread_mutex_t mutexRound;
    pthread_cond_t condRound;

    // int win;
    pthread_mutex_t mutexDeal;
    pthread_cond_t condDeal;

};

void* initPlayer(void *arg) {
    int* i = (int*)arg;
    // player
}

void startRound() {

}

void endRound() {

}

int selectCM(deck_t* deck) {
    int randNum = rand() % ((deck->size - 1)+ 1);
    int cookieMaster = deck->cards[randNum];
    deck->size--;
    for (int i = randNum; i < deck->size; i++) {
        deck->cards[i] = deck->cards[i + 1];
    }
    return cookieMaster;
}

int selectTop(deck_t* deck) {
    int topCard = deck->cards[0];
    deck->size--;
    for (int i = 0; i < deck->size; i++) {
        deck->cards[i] = deck->cards[i + 1];
    }
    return topCard;
}

void* turn(deck_t* deck) {
    // printf("%d \n",deck->size);
    // shuffle(deck->cards, deck->size);

    int randNum = rand() % ((deck->size - 1)+ 1);
    // int cookieMaster = deck->cards[randNum];
    // deck->size--;

    // for (int i = randNum; i < deck->size; i++) {
    //     deck->cards[i] = deck->cards[i + 1];
    // }

    // pthread_mutex_t mutex;
    // pthread_mutex_unlock(&mutex);


    // printf("%d \n",deck->size);
    // for(int i= 0; i < deck->size; i++) {
    //     printf("%d ", deck->cards[i]);
    // }
}

void shuffle(gameData_t* gameData) {
    deck_t* deck = &gameData->deck;

    if (deck->size > 1) {
        pthread_mutex_lock(&gameData->mutexShuffled);
        while (deck->shuffled) {
            pthread_cond_wait(&gameData->condShuffled, &gameData->mutexShuffled);
        }

        for (size_t i = 0; i < deck->size - 1; i++) {
            size_t j = i + rand() / (RAND_MAX / (deck->size - i) + 1);
            int t = deck->cards[j];
            deck->cards[j] = deck->cards[i];
            deck->cards[i] = t;
            deck->shuffled = 1;
        }

        pthread_cond_signal(&gameData->condShuffled);
        pthread_mutex_unlock(&gameData->mutexShuffled);
    }
    return;
}

void choose(gameData_t* gameData) {
    deck_t* deck = &gameData->deck;
    
    pthread_mutex_lock(&gameData->mutexShuffled);
    while (!deck->shuffled) {
        printf("Choose Waiting...\n");
        pthread_cond_wait(&gameData->condShuffled, &gameData->mutexShuffled);
    }
    deck->shuffled = 0;
    pthread_cond_signal(&gameData->condShuffled);
    pthread_mutex_unlock(&gameData->mutexShuffled);

    gameData->cookieMaster = selectCM(deck);

    pthread_cond_signal(&gameData->condMaster);
    pthread_mutex_unlock(&gameData->mutexMaster);

    return;
}

void deal(gameData_t* gameData) {
    deck_t* deck = &gameData->deck;

    pthread_mutex_lock(&gameData->mutexMaster);
    while(gameData->cookieMaster < 0) {
        printf("Deal Waiting...\n");
        pthread_cond_wait(&gameData->condMaster, &gameData->mutexMaster);
    }
    pthread_mutex_unlock(&gameData->mutexMaster);

    for(int i = gameData->currentDealer + 1; i % gameData->numPlayers != gameData->currentDealer % gameData->numPlayers; i++) {
        int topCard = selectTop(deck);
        int handLen = gameData->players[i].handLen;

        gameData->players[i].hand[handLen] = topCard;
        gameData->players[i].handLen++;

        // for(int j = 0; j < handLen; j++) {
        //     if(gameData->players[i].hand[j] == gameData->cookieMaster) {
        //         printf("WINNER: %d  %d\n", gameData->players[i].hand[j] , gameData->players[i].hand[handLen]);
        //     }
        // }
    }

    // pthread_mutex_lock(&gameData->mutexTurn);
    // pthread_cond_signal(&gameData->condTurn);
    // pthread_mutex_unlock(&gameData->mutexTurn);

    return;
}

void compareHand(int cookieMaster, player_t* player) {

}

void* play(void *arg) {
    player_t* player = (player_t*)arg;
    gameData_t* gameData = player->gameData;
    int id = player->id;
    printf("Thread for player %d started\n", player->id);

    for (int round = 0; round < gameData->numPlayers; round++) {
        pthread_mutex_lock(&gameData->mutexRound);
        printf("RUNNING THREAD:  %d\n", id);

        while(id != gameData->currentDealer) {
            // printf("Player %d is dealer: Waiting for current dealer to be %d...\n", id, gameData->currentDealer);
            pthread_cond_wait(&gameData->condRound, &gameData->mutexRound);
        }

        if(id == gameData->currentDealer) {
            // printf("Player %d is dealer: ID %d...\n", gameData->currentDealer, id);
            // printf("Player %d is dealer\n", gameData->currentDealer);

            // pthread_mutex_unlock(&gameData->mutexTurn);

            shuffle(gameData);
            choose(gameData);
            // deal(gameData);

            // pthread_mutex_lock(&gameData->mutexTurn);
            gameData->currentPlayer = (gameData->currentPlayer + 1) % gameData->numPlayers;
            // printf("Player %d is dealer: currentPlayer %d...\n", gameData->currentDealer, gameData->currentPlayer);
            pthread_cond_signal(&gameData->condRound);
        }
        pthread_mutex_unlock(&gameData->mutexRound);
        // else pthread_mutex_unlock(&gameData->mutexTurn);

        // printf("Player %d is dealer: current player is %d...\n", gameData->currentDealer);
        pthread_mutex_lock(&gameData->mutexTurn);
        while (id == gameData->currentPlayer) {
            printf("Player %d is dealer: Waiting for ID %d to be %d...\n", gameData->currentDealer, id, gameData->currentPlayer);
            pthread_cond_wait(&gameData->condTurn, &gameData->mutexTurn);
        }
        // printf("Player %d is dealer. Player %d taking turn\n", gameData->currentDealer, gameData->currentPlayer);
        printf("Player %d is dealer: ID %d with current player %d...\n", gameData->currentDealer, id, gameData->currentPlayer);

        gameData->currentPlayer = (gameData->currentPlayer + 1) % gameData->numPlayers;
        pthread_cond_signal(&gameData->condTurn);
        pthread_mutex_unlock(&gameData->mutexTurn);

        pthread_mutex_lock(&gameData->mutexTurn);

        printf("Player %d is dealer: current player changed to %d...\n", gameData->currentDealer, gameData->currentPlayer);
        if (gameData->currentPlayer == gameData->currentDealer) {
            gameData->currentDealer = (gameData->currentDealer + 1) % gameData->numPlayers;
            gameData->currentPlayer = gameData->currentDealer;
            printf("Dealer changed to %d: current player changed to %d...\n\n", gameData->currentDealer, gameData->currentPlayer);
            pthread_cond_signal(&gameData->condTurn);
        } 
        // pthread_cond_signal(&gameData->condTurn);
        pthread_mutex_unlock(&gameData->mutexTurn);

        // if(gameData->currentDealer == )
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    // for (int i = 0; i < argc; ++i) {
    //     printf("argv[%d]: %s\n", i, argv[i]);
    // }
    int seed = atoi(argv[1]);
    int numPlayers = atoi(argv[2]);
    int numCookies = atoi(argv[3]);
    srand(seed);

    deck_t deck;
    deck.cards[DECK_START_AMT];
    deck.size = DECK_START_AMT;
    deck.shuffled = 0;

    for(int i = 0; i < DECK_START_AMT; i++) {
        deck.cards[i] = (i / 4) + 1;
    }

    player_t players[numPlayers];

    gameData_t gameData;
    gameData.numPlayers = numPlayers;
    gameData.numCookies = numCookies;
    gameData.cookieMaster = -1;
    gameData.deck = deck;
    gameData.players = players;
    gameData.currentDealer = 0;
    gameData.currentPlayer = 0;
    gameData.turn = 0;


    pthread_mutex_t mutexShuffled, mutexMaster, mutexTurn, mutexRound, mutexDeal = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t condShuffled, condMaster, condTurn, condRound, condDeal = PTHREAD_COND_INITIALIZER;

    pthread_mutex_init(&mutexShuffled, NULL);
    pthread_mutex_init(&mutexMaster, NULL);
    pthread_mutex_init(&mutexTurn, NULL);
    pthread_mutex_init(&mutexRound, NULL);
    pthread_mutex_init(&mutexDeal, NULL);
    pthread_cond_init(&condShuffled, NULL);
    pthread_cond_init(&condMaster, NULL);
    pthread_cond_init(&condTurn, NULL);
    pthread_cond_init(&condRound, NULL);
    pthread_cond_init(&condDeal, NULL);

    gameData.mutexShuffled = mutexShuffled;
    gameData.mutexMaster = mutexMaster;
    gameData.mutexTurn = mutexTurn;
    gameData.mutexDeal = mutexDeal;
    gameData.condShuffled = condShuffled;
    gameData.condMaster = condMaster;
    gameData.condTurn = condTurn;
    gameData.condDeal = condDeal;

    // for(int i = 0; i < numPlayers; i++) {
    //     players[i].id = i;

    // }



    gameData.players = malloc(sizeof(player_t) * gameData.numPlayers);

    for(int i = 0; i < numPlayers; i++) {
        players[i].id = i;
        players[i].handLen = 0;
        players[i].gameData = &gameData;

        pthread_create(&players[i % numPlayers].thread, NULL, play, &players[i % numPlayers]);
    }
    for(int i = 0; i < numPlayers; i++) {
        pthread_join(players[i % numPlayers].thread, NULL);
    }

    // for(int i = 0; i < numPlayers; i++) {
    //     // for(int i = 0; i < 5; i++) {
    //         gameData.mutex = mutex;
    //         gameData.cond = cond;
    //         pthread_create(&players[i % numPlayers].thread, NULL, shuffle, &deck);
    //         pthread_create(&players[(i + 1) % numPlayers].thread, NULL, choose, &deck);

    //         pthread_join(players[i % numPlayers].thread, NULL);
    //         pthread_join(players[(i + 1) % numPlayers].thread, NULL);

    //         gameData.mutex = mutex;

    //     // }

    //     printf("here: %d\n", gameData.cookieMaster);


    
    //     // players[i] = (player){.id=i, .thread=NULL, .hand=NULL, .handLen=0};
    //     // players[i].id = i;
    //     // printf("i: %d\n", i);
    //     // pthread_create(&players[i].thread, NULL, routine, &mutexShuffle);
    //     // players[i].hand = NULL;
    //     // players[i].handLen = 0;
    // }
    printf("fin: %d\n", players[0].id);
    // pthread_mutex_destroy(&deck.mutex);
    // pthread_cond_destroy(&deck.cond);

    // pthread_mutex_destroy(&mutex);
    // pthread_cond_destroy(&cond);

    int playerTurn = 0;
    int winnerId = 0;

    // pthread_cond_signal(&condCookie);
    // pthread_cond_wait(&condCookie, &mutexCookie);

    // printf("deal\n");
    // deal(players, numPlayers);

    // Free dynamic array of ints in each player
    // for (size_t i = 0; i < numPlayers; i++) {
    //     // printf("Free player %d's hand\n", i);
    //     free(players[i].hand);
    // }
    return 0;
}